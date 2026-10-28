#include "OnvifDiscovery.h"

#include "OnvifParse.h"
#include "../../core/Logger.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <sstream>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <bcrypt.h>

namespace cameras::onvif {

namespace {

struct SocketGuard {
    SOCKET socket = INVALID_SOCKET;
    ~SocketGuard() {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
        }
    }
};

bool startupWinsock() {
    static bool started = false;
    if (started) {
        return true;
    }
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
    started = true;
    return true;
}

std::string messageId() {
    unsigned char bytes[16];
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        return "uuid:00000000-0000-4000-8000-000000000001";
    }
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);
    char buf[48];
    std::snprintf(buf, sizeof(buf),
                  "uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
                  bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return buf;
}

std::string probeXml(const std::string& types) {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<e:Envelope xmlns:e=\"http://www.w3.org/2003/05/soap-envelope\" "
        << "xmlns:w=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
        << "xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
        << "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" "
        << "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">"
        << "<e:Header>"
        << "<w:MessageID>" << messageId() << "</w:MessageID>"
        << "<w:To e:mustUnderstand=\"true\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</w:To>"
        << "<w:Action e:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</w:Action>"
        << "</e:Header><e:Body><d:Probe><d:Types>" << types << "</d:Types></d:Probe></e:Body></e:Envelope>";
    return xml.str();
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word.push_back(c);
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

std::string scopeValue(const std::string& scopes, const std::string& key) {
    for (const auto& scope : splitWords(scopes)) {
        const size_t pos = scope.find(key);
        if (pos != std::string::npos) {
            return urlDecode(scope.substr(pos + key.size()));
        }
    }
    return {};
}

std::string labelFrom(const std::string& name, const std::string& hardware) {
    if (!name.empty() && !hardware.empty() && name.find(hardware) == std::string::npos) {
        return name + " " + hardware;
    }
    if (!name.empty()) {
        return name;
    }
    return hardware;
}

bool looksLikeOnvif(const std::string& types, const std::string& path) {
    if (types.find("NetworkVideoTransmitter") != std::string::npos) {
        return true;
    }
    std::string lower = path;
    for (char& c : lower) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return lower.find("onvif") != std::string::npos;
}

void considerMatch(const std::string& block, std::map<std::string, OnvifEndpoint>& byAddress) {
    const std::string xaddrs = xmlElementText(block, "XAddrs");
    const std::string scopes = xmlElementText(block, "Scopes");
    const std::string types = xmlElementText(block, "Types");
    if (xaddrs.empty()) {
        return;
    }

    const std::string name = scopeValue(scopes, "/name/");
    const std::string hardware = scopeValue(scopes, "/hardware/");
    const std::string label = labelFrom(name, hardware);

    OnvifEndpoint chosen;
    bool found = false;
    for (const auto& word : splitWords(xaddrs)) {
        const auto url = parseHttpUrl(word);
        if (!url || url->https || url->host.empty()) {
            continue;
        }
        OnvifEndpoint endpoint;
        endpoint.host = url->host;
        endpoint.port = url->port;
        endpoint.servicePath = url->path.empty() ? "/onvif/device_service" : url->path;
        if (!looksLikeOnvif(types, endpoint.servicePath)) {
            continue;
        }
        endpoint.label = label;
        const bool deviceService = endpoint.servicePath.find("device_service") != std::string::npos;
        if (!found || deviceService) {
            chosen = std::move(endpoint);
            found = true;
            if (deviceService) {
                break;
            }
        }
    }
    if (!found) {
        return;
    }
    const auto existing = byAddress.find(chosen.address());
    if (existing == byAddress.end()) {
        byAddress.emplace(chosen.address(), std::move(chosen));
        return;
    }
    const bool chosenIsDevice = chosen.servicePath.find("device_service") != std::string::npos;
    const bool existingIsDevice = existing->second.servicePath.find("device_service") != std::string::npos;
    if (chosenIsDevice && !existingIsDevice) {
        existing->second = std::move(chosen);
    } else if (existing->second.label.empty()) {
        existing->second.label = chosen.label;
    }
}

void absorbDatagram(const std::string& xml, std::map<std::string, OnvifEndpoint>& byAddress) {
    size_t pos = 0;
    while ((pos = xml.find("ProbeMatch", pos)) != std::string::npos) {
        if (pos + 10 < xml.size() && xml[pos + 10] == 'e') {
            pos += 11;
            continue;
        }
        const size_t next = xml.find("ProbeMatch", pos + 10);
        const std::string block = next == std::string::npos
            ? xml.substr(pos)
            : xml.substr(pos, next - pos);
        considerMatch(block, byAddress);
        if (next == std::string::npos) {
            break;
        }
        pos = next;
    }
}

std::vector<in_addr> localInterfaces() {
    std::vector<in_addr> addresses;
    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer(size);
    auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG result = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &size);
    if (result == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(size);
        adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        result = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &size);
    }
    if (result != NO_ERROR) {
        return addresses;
    }
    for (auto* adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        for (auto* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
            if (unicast->Address.lpSockaddr == nullptr
                || unicast->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }
            addresses.push_back(reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr)->sin_addr);
        }
    }
    return addresses;
}

void sendProbe(SOCKET socket, const sockaddr_in& dest, const std::string& payload) {
    sendto(socket, payload.data(), static_cast<int>(payload.size()), 0,
           reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
}

} // namespace

OnvifScanResult OnvifDiscovery::probe(int timeoutMs) {
    OnvifScanResult result;
    if (timeoutMs < 500) {
        timeoutMs = 500;
    }
    if (!startupWinsock()) {
        result.error = "could not start network discovery";
        return result;
    }

    SocketGuard guard;
    guard.socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (guard.socket == INVALID_SOCKET) {
        result.error = "could not open a discovery socket";
        return result;
    }

    BOOL reuse = TRUE;
    setsockopt(guard.socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = 0;
    if (bind(guard.socket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
        result.error = "could not bind the discovery socket";
        return result;
    }

    int ttl = 2;
    setsockopt(guard.socket, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl));
    DWORD receiveTimeout = 300;
    setsockopt(guard.socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&receiveTimeout), sizeof(receiveTimeout));

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(3702);
    if (InetPtonA(AF_INET, "239.255.255.250", &dest.sin_addr) != 1) {
        result.error = "could not build the discovery address";
        return result;
    }

    const std::string nvt = probeXml("dn:NetworkVideoTransmitter");
    const std::string device = probeXml("tds:Device");
    auto interfaces = localInterfaces();
    if (interfaces.empty()) {
        in_addr any{};
        any.s_addr = htonl(INADDR_ANY);
        interfaces.push_back(any);
    }
    core::Logger::info("ONVIF discovery probing " + std::to_string(interfaces.size()) + " interface(s)");

    auto sendAll = [&]() {
        for (const auto& iface : interfaces) {
            setsockopt(guard.socket, IPPROTO_IP, IP_MULTICAST_IF,
                       reinterpret_cast<const char*>(&iface), sizeof(iface));
            sendProbe(guard.socket, dest, nvt);
            sendProbe(guard.socket, dest, device);
        }
    };
    sendAll();

    std::map<std::string, OnvifEndpoint> found;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    bool resent = false;
    std::vector<char> buffer(16 * 1024);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!resent && std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs / 2) > deadline) {
            sendAll();
            resent = true;
        }
        sockaddr_in from{};
        int fromLen = sizeof(from);
        const int received = recvfrom(guard.socket, buffer.data(), static_cast<int>(buffer.size()), 0,
                                      reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (received == SOCKET_ERROR) {
            const int code = WSAGetLastError();
            if (code == WSAETIMEDOUT || code == WSAEWOULDBLOCK) {
                continue;
            }
            break;
        }
        if (received > 0) {
            absorbDatagram(std::string(buffer.data(), buffer.data() + received), found);
        }
    }

    result.devices.reserve(found.size());
    for (auto& entry : found) {
        result.devices.push_back(std::move(entry.second));
    }
    std::sort(result.devices.begin(), result.devices.end(), [](const OnvifEndpoint& a, const OnvifEndpoint& b) {
        if (a.host != b.host) {
            return a.host < b.host;
        }
        return a.port < b.port;
    });
    core::Logger::info("ONVIF discovery found " + std::to_string(result.devices.size()) + " camera(s)");
    return result;
}

} // namespace cameras::onvif
