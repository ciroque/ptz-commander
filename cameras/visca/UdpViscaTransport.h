#ifndef CAMERAS_VISCA_UDP_VISCA_TRANSPORT_H
#define CAMERAS_VISCA_UDP_VISCA_TRANSPORT_H

#include "ViscaTransport.h"
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace cameras::visca {

class UdpViscaTransport : public ViscaTransport {
public:
    UdpViscaTransport(std::string host, int port = 52381);
    ~UdpViscaTransport() override;

    bool open() override;
    bool isOpen() const override;
    void close() override;
    bool write(const std::vector<uint8_t>& data) override;
    void purgeInput() override;
    std::vector<uint8_t> readPacket(int timeoutMs = 1000) override;

    const std::string& host() const { return host_; }
    int port() const { return port_; }

private:
    std::vector<uint8_t> frame(const std::vector<uint8_t>& visca);
    static std::vector<uint8_t> unframe(const std::vector<uint8_t>& datagram);
    static std::string toHex(const std::vector<uint8_t>& data);

    std::string host_;
    int port_ = 52381;
    uint32_t sequence_ = 0;
#ifdef _WIN32
    SOCKET socket_ = INVALID_SOCKET;
    sockaddr_in dest_{};
#endif
};

} // namespace cameras::visca

#endif
