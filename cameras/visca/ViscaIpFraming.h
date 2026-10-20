#ifndef CAMERAS_VISCA_VISCA_IP_FRAMING_H
#define CAMERAS_VISCA_VISCA_IP_FRAMING_H

#include <cstdint>
#include <vector>

namespace cameras::visca {

// VISCA-over-IP wraps each VISCA message in an 8-byte header:
// payload type (0x0100 command, 0x0110 inquiry, 0x0111 reply), payload
// length, and a 32-bit sequence number, all big-endian.
inline std::vector<uint8_t> viscaIpFrame(const std::vector<uint8_t>& visca, uint32_t sequence) {
    const uint16_t type = (visca.size() >= 2 && visca[1] == 0x09) ? 0x0110 : 0x0100;
    const uint16_t len = static_cast<uint16_t>(visca.size());
    std::vector<uint8_t> out;
    out.reserve(8 + visca.size());
    out.push_back(static_cast<uint8_t>(type >> 8));
    out.push_back(static_cast<uint8_t>(type & 0xFF));
    out.push_back(static_cast<uint8_t>(len >> 8));
    out.push_back(static_cast<uint8_t>(len & 0xFF));
    out.push_back(static_cast<uint8_t>(sequence >> 24));
    out.push_back(static_cast<uint8_t>(sequence >> 16));
    out.push_back(static_cast<uint8_t>(sequence >> 8));
    out.push_back(static_cast<uint8_t>(sequence & 0xFF));
    out.insert(out.end(), visca.begin(), visca.end());
    return out;
}

// Strips the header if present; some cameras reply with bare VISCA.
inline std::vector<uint8_t> viscaIpUnframe(const std::vector<uint8_t>& datagram) {
    if (datagram.size() >= 8 && datagram[0] == 0x01 &&
        (datagram[1] == 0x00 || datagram[1] == 0x01 || datagram[1] == 0x11 || datagram[1] == 0x10)) {
        return std::vector<uint8_t>(datagram.begin() + 8, datagram.end());
    }
    return datagram;
}

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_IP_FRAMING_H
