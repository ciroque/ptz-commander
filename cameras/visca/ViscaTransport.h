#ifndef CAMERAS_VISCA_VISCA_TRANSPORT_H
#define CAMERAS_VISCA_VISCA_TRANSPORT_H

#include <cstdint>
#include <vector>

namespace cameras::visca {

class ViscaTransport {
public:
    virtual ~ViscaTransport() = default;

    virtual bool open() = 0;
    virtual bool isOpen() const = 0;
    virtual void close() = 0;

    // Fully formed VISCA payload (address byte ... 0xFF). Framing belongs in the transport.
    virtual bool write(const std::vector<uint8_t>& data) = 0;
    virtual void purgeInput() = 0;
    virtual std::vector<uint8_t> readPacket(int timeoutMs = 1000) = 0;
};

} // namespace cameras::visca

#endif
