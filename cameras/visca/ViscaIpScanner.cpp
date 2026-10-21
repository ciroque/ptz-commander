#include "ViscaIpScanner.h"
#include "ViscaIpFraming.h"
#include "../../core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
#endif

#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <thread>

#ifdef _WIN32

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace cameras::visca {

namespace {
    using Clock = std::chrono::steady_clock;

    // Probes go out in timed batches (about 20k/s), so a large range doesn't
    // burst tens of thousands of packets at once. Paced by the clock rather
    // than by short waits: Windows rounds those up to its ~15.6 ms tick.
    constexpr size_t kBatchSize = 512;
    constexpr auto kBatchInterval = std::chrono::milliseconds(25);
    // Every host is probed twice in case the first probe is lost, e.g.
    // while the PC is still resolving the host's MAC address.
    constexpr int kRounds = 2;
    // The pause between rounds, listening all the while. Windows queues few
    // datagrams per host while ARP resolves, so a retry sent straight after
    // the first probe can be dropped with it; after the pause the address
    // is cached and the retry is a separate transmission.
    constexpr auto kRoundGap = std::chrono::milliseconds(250);
    // How long to keep listening after the last probe.
    constexpr auto kReplyWindow = std::chrono::milliseconds(1500);
    // Consecutive ICMP resets tolerated before receiving counts as broken.
    constexpr int kMaxResetsInARow = 16;

    // Clamped so an out-of-range prefix can't shift by a negative or full-width count.
    uint32_t maskFor(int prefix) {
        if (prefix <= 0) {
            return 0;
        }
        return prefix >= 32 ? 0xFFFFFFFFu : 0xFFFFFFFFu << (32 - prefix);
    }

    class WinsockSession {
    public:
        WinsockSession() { ok_ = WSAStartup(MAKEWORD(2, 2), &data_) == 0; }
        ~WinsockSession() { if (ok_) WSACleanup(); }
        bool ok() const { return ok_; }
    private:
        WSADATA data_{};
        bool ok_ = false;
    };
}

std::vector<uint32_t> Ipv4Network::hosts() const {
    if (prefix < kMinScanPrefix || prefix > 32) {
        return {};
    }
    const uint64_t size = uint64_t{1} << (32 - prefix);
    std::vector<uint32_t> out;
    if (prefix >= 31) {
        for (uint64_t i = 0; i < size; ++i) {
            out.push_back(base + static_cast<uint32_t>(i));
        }
        return out;
    }
    out.reserve(static_cast<size_t>(size - 2));
    for (uint64_t i = 1; i + 1 < size; ++i) {
        out.push_back(base + static_cast<uint32_t>(i));
    }
    return out;
}

std::string ipv4ToString(uint32_t address) {
    return std::to_string(address >> 24) + "." + std::to_string((address >> 16) & 0xFF) + "." +
           std::to_string((address >> 8) & 0xFF) + "." + std::to_string(address & 0xFF);
}

std::string Ipv4Network::toString() const {
    return ipv4ToString(base) + "/" + std::to_string(prefix);
}

std::optional<Ipv4Network> parseIpv4Network(const std::string& text, std::string& error) {
    const auto slash = text.find('/');
    const std::string addressText = text.substr(0, slash);
    int prefix = 32;
    if (slash != std::string::npos) {
        const std::string prefixText = text.substr(slash + 1);
        if (prefixText.empty() || prefixText.size() > 2 ||
            !std::all_of(prefixText.begin(), prefixText.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            error = "invalid prefix length in '" + text + "'";
            return std::nullopt;
        }
        prefix = std::stoi(prefixText);
        if (prefix > 32) {
            error = "invalid prefix length in '" + text + "'";
            return std::nullopt;
        }
    }

    in_addr addr{};
    if (InetPtonA(AF_INET, addressText.c_str(), &addr) != 1) {
        error = "'" + addressText + "' is not an IPv4 address";
        return std::nullopt;
    }
    if (prefix < kMinScanPrefix) {
        error = "/" + std::to_string(prefix) + " is too large to scan; use /" +
                std::to_string(kMinScanPrefix) + " or smaller";
        return std::nullopt;
    }
    return Ipv4Network{ntohl(addr.s_addr) & maskFor(prefix), prefix};
}

std::vector<Ipv4Network> localIpv4Networks() {
    std::vector<Ipv4Network> networks;
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer;
    ULONG result = ERROR_BUFFER_OVERFLOW;
    // The adapter list can grow between the sizing call and the real one.
    for (int attempt = 0; attempt < 3 && result == ERROR_BUFFER_OVERFLOW; ++attempt) {
        buffer.resize(size);
        result = GetAdaptersAddresses(AF_INET,
                                      GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                      nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
    }
    if (result != NO_ERROR) {
        core::Logger::error("VISCA-IP scan: GetAdaptersAddresses failed (" + std::to_string(result) + ")");
        return networks;
    }

    for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        for (auto* ua = adapter->FirstUnicastAddress; ua; ua = ua->Next) {
            if (ua->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }
            const auto* sin = reinterpret_cast<const sockaddr_in*>(ua->Address.lpSockaddr);
            const uint32_t address = ntohl(sin->sin_addr.s_addr);
            if ((address >> 16) == 0xA9FE) {  // 169.254/16: no DHCP lease, nothing to find
                continue;
            }
            // Windows reports 255 when the prefix isn't known; anything outside
            // 1..32 is invalid for IPv4, so fall back to the /24.
            const int onLink = ua->OnLinkPrefixLength;
            const int prefix = (onLink >= 1 && onLink <= 32) ? std::max(onLink, kLocalScanPrefix)
                                                              : kLocalScanPrefix;
            const Ipv4Network network{address & maskFor(prefix), prefix};
            const bool seen = std::any_of(networks.begin(), networks.end(), [&](const Ipv4Network& n) {
                return n.base == network.base && n.prefix == network.prefix;
            });
            if (!seen) {
                networks.push_back(network);
            }
        }
    }
    return networks;
}

std::vector<ViscaIpScanHit> scanViscaIp(const std::vector<Ipv4Network>& networks, int port,
                                        std::string& error) {
    WinsockSession winsock;
    if (!winsock.ok()) {
        error = "could not start Winsock";
        return {};
    }

    SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        error = "could not open a UDP socket";
        return {};
    }
    // Hosts without a listener answer with ICMP port unreachable, which
    // Windows otherwise reports as WSAECONNRESET on the next recvfrom.
    BOOL reportReset = FALSE;
    DWORD returned = 0;
    if (WSAIoctl(sock, SIO_UDP_CONNRESET, &reportReset, sizeof(reportReset), nullptr, 0, &returned, nullptr,
                 nullptr) != 0) {
        error = "could not turn off ICMP reset reporting (WSA error " + std::to_string(WSAGetLastError()) + ")";
        closesocket(sock);
        return {};
    }
    int receiveBuffer = 1 << 20;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&receiveBuffer), sizeof(receiveBuffer));

    std::vector<uint32_t> targets;
    for (const auto& network : networks) {
        const auto hosts = network.hosts();
        targets.insert(targets.end(), hosts.begin(), hosts.end());
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    const std::set<uint32_t> targetSet(targets.begin(), targets.end());

    std::map<uint32_t, ViscaIpScanHit> hits;
    // Set when receiving fails in a way that could repeat forever.
    std::string receiveError;
    int resetsInARow = 0;
    // Returns true while there may be more to read without waiting.
    const auto receive = [&](int timeoutMs) {
        timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) {
            return false;
        }
        uint8_t buf[256];
        sockaddr_in from{};
        int fromLen = sizeof(from);
        const int n = recvfrom(sock, reinterpret_cast<char*>(buf), sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n < 0) {
            const int wsaError = WSAGetLastError();
            if (wsaError == WSAEMSGSIZE) {
                return true;  // an oversized datagram, discarded; not a VISCA reply
            }
            // A stray ICMP reset is skipped, but a run of them means the
            // error isn't clearing and the drain would never end.
            if ((wsaError == WSAECONNRESET || wsaError == WSAENETRESET) && ++resetsInARow <= kMaxResetsInARow) {
                return true;
            }
            receiveError = "receive failed (WSA error " + std::to_string(wsaError) + ")";
            return false;
        }
        resetsInARow = 0;
        if (n == 0) {
            return true;
        }
        const uint32_t host = ntohl(from.sin_addr.s_addr);
        const auto visca = viscaIpUnframe(std::vector<uint8_t>(buf, buf + n));
        if (!targetSet.count(host) || !isViscaReply(visca)) {
            return true;
        }
        auto& hit = hits[host];
        if (hit.reply.empty() || (!hit.position && ViscaCommands::parsePanTiltPosition(visca))) {
            hit.host = ipv4ToString(host);
            hit.port = port;
            hit.reply = visca;
            hit.position = ViscaCommands::parsePanTiltPosition(visca);
        }
        return true;
    };

    const auto listenUntil = [&](Clock::time_point deadline) {
        while (receiveError.empty() && Clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
            receive(static_cast<int>(std::max<long long>(1, remaining.count())));
        }
    };

    const auto probe = viscaIpFrame(ViscaCommands::panTiltPositionInquiry(1), 0);
    auto batchStart = Clock::now();
    for (int round = 0; round < kRounds && receiveError.empty(); ++round) {
        if (round > 0) {
            listenUntil(Clock::now() + kRoundGap);
            batchStart = Clock::now();
        }
        for (size_t i = 0; i < targets.size() && receiveError.empty(); ++i) {
            sockaddr_in dest{};
            dest.sin_family = AF_INET;
            dest.sin_port = htons(static_cast<u_short>(port));
            dest.sin_addr.s_addr = htonl(targets[i]);
            sendto(sock, reinterpret_cast<const char*>(probe.data()), static_cast<int>(probe.size()), 0,
                   reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
            if ((i + 1) % kBatchSize == 0) {
                while (receive(0)) {}  // drain what has arrived, without waiting
                std::this_thread::sleep_until(batchStart + kBatchInterval);
                batchStart = Clock::now();
            }
        }
    }

    listenUntil(Clock::now() + kReplyWindow);
    closesocket(sock);
    if (!receiveError.empty()) {
        error = receiveError;
        return {};
    }

    std::vector<ViscaIpScanHit> out;
    for (auto& [host, hit] : hits) {
        out.push_back(std::move(hit));
    }
    core::Logger::info("VISCA-IP scan: probed " + std::to_string(targets.size()) + " hosts on port " +
                       std::to_string(port) + ", " + std::to_string(out.size()) + " replied");
    return out;
}

} // namespace cameras::visca

#endif // _WIN32
