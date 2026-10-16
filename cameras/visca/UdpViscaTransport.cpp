#include "UdpViscaTransport.h"
#include "../../core/Logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <sstream>
#include <iomanip>

#ifdef _WIN32

#pragma comment(lib, "ws2_32.lib")

namespace cameras::visca {

namespace {
    bool wsaReady() {
        static bool started = false;
        static WSADATA wsa{};
        if (started) {
            return true;
        }
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            return false;
        }
        started = true;
        return true;
    }
}

UdpViscaTransport::UdpViscaTransport(std::string host, int port)
    : host_(std::move(host)), port_(port) {}

UdpViscaTransport::~UdpViscaTransport() {
    close();
}

bool UdpViscaTransport::open() {
    close();
    if (!wsaReady()) {
        core::Logger::error("VISCA-IP WSAStartup failed");
        return false;
    }

    socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        core::Logger::error("VISCA-IP socket() failed");
        return false;
    }

    dest_ = {};
    dest_.sin_family = AF_INET;
    dest_.sin_port = htons(static_cast<u_short>(port_));
    if (InetPtonA(AF_INET, host_.c_str(), &dest_.sin_addr) != 1) {
        core::Logger::error("VISCA-IP invalid host " + host_);
        close();
        return false;
    }

    core::Logger::info("VISCA-IP UDP ready " + host_ + ":" + std::to_string(port_));
    return true;
}

bool UdpViscaTransport::isOpen() const {
    return socket_ != INVALID_SOCKET;
}

void UdpViscaTransport::close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

std::string UdpViscaTransport::toHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (size_t i = 0; i < data.size(); ++i) {
        if (i) oss << ' ';
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> UdpViscaTransport::frame(const std::vector<uint8_t>& visca) {
    const uint16_t type = (visca.size() >= 2 && visca[1] == 0x09) ? 0x0110 : 0x0100;
    const uint16_t len = static_cast<uint16_t>(visca.size());
    const uint32_t seq = sequence_++;
    std::vector<uint8_t> out;
    out.reserve(8 + visca.size());
    out.push_back(static_cast<uint8_t>(type >> 8));
    out.push_back(static_cast<uint8_t>(type & 0xFF));
    out.push_back(static_cast<uint8_t>(len >> 8));
    out.push_back(static_cast<uint8_t>(len & 0xFF));
    out.push_back(static_cast<uint8_t>(seq >> 24));
    out.push_back(static_cast<uint8_t>(seq >> 16));
    out.push_back(static_cast<uint8_t>(seq >> 8));
    out.push_back(static_cast<uint8_t>(seq & 0xFF));
    out.insert(out.end(), visca.begin(), visca.end());
    return out;
}

std::vector<uint8_t> UdpViscaTransport::unframe(const std::vector<uint8_t>& datagram) {
    if (datagram.size() >= 8 && datagram[0] == 0x01 &&
        (datagram[1] == 0x00 || datagram[1] == 0x01 || datagram[1] == 0x11 || datagram[1] == 0x10)) {
        return std::vector<uint8_t>(datagram.begin() + 8, datagram.end());
    }
    return datagram;
}

bool UdpViscaTransport::write(const std::vector<uint8_t>& data) {
    if (!isOpen() || data.empty()) {
        return false;
    }
    const auto pkt = frame(data);
    core::Logger::debug("VISCA-IP TX " + host_ + ":" + std::to_string(port_) + " " + toHex(pkt));
    const int sent = sendto(socket_, reinterpret_cast<const char*>(pkt.data()),
                            static_cast<int>(pkt.size()), 0,
                            reinterpret_cast<const sockaddr*>(&dest_), sizeof(dest_));
    return sent == static_cast<int>(pkt.size());
}

void UdpViscaTransport::purgeInput() {
    if (!isOpen()) {
        return;
    }
    u_long mode = 1;
    ioctlsocket(socket_, FIONBIO, &mode);
    char buf[256];
    sockaddr_in from{};
    int fromLen = sizeof(from);
    while (recvfrom(socket_, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fromLen) > 0) {
        fromLen = sizeof(from);
    }
    mode = 0;
    ioctlsocket(socket_, FIONBIO, &mode);
}

std::vector<uint8_t> UdpViscaTransport::readPacket(int timeoutMs) {
    if (!isOpen()) {
        return {};
    }

    DWORD start = GetTickCount();
    while (GetTickCount() - start < static_cast<DWORD>(timeoutMs) + 50) {
        const DWORD remaining = timeoutMs - (GetTickCount() - start);
        if (static_cast<int>(remaining) <= 0) {
            break;
        }
        timeval tv{};
        tv.tv_sec = static_cast<long>(remaining / 1000);
        tv.tv_usec = static_cast<long>((remaining % 1000) * 1000);
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(socket_, &fds);
        const int sel = select(0, &fds, nullptr, nullptr, &tv);
        if (sel <= 0) {
            break;
        }
        uint8_t buf[256];
        sockaddr_in from{};
        int fromLen = sizeof(from);
        const int n = recvfrom(socket_, reinterpret_cast<char*>(buf), sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n <= 0) {
            break;
        }
        std::vector<uint8_t> datagram(buf, buf + n);
        core::Logger::debug("VISCA-IP RX " + host_ + ":" + std::to_string(port_) + " " + toHex(datagram));
        auto visca = unframe(datagram);
        if (visca.size() >= 3 && visca[0] == 0x90 && visca[1] == 0x41) {
            continue; // ACK; wait for completion or inquiry payload
        }
        if (!visca.empty() && visca.back() == 0xFF) {
            return visca;
        }
    }
    return {};
}

} // namespace cameras::visca

#endif // _WIN32
