#ifndef CAMERAS_VISCA_SERIAL_TRANSPORT_H
#define CAMERAS_VISCA_SERIAL_TRANSPORT_H

#include <cstdint>
#include <string>
#include <vector>

namespace cameras::visca {

// Abstract serial transport so ViscaCamera / commands are not coupled to Win32 COM APIs.
// Future: can add a POSIX (termios) implementation for Linux/macOS VISCA serial.

class SerialTransport {
public:
    virtual ~SerialTransport() = default;

    // Open the port (e.g. "COM3" on Windows, "/dev/ttyUSB0" on *nix).
    // baud: e.g. 9600, 38400, etc.
    // Returns true on success.
    virtual bool open(const std::string& port, int baud) = 0;

    virtual bool isOpen() const = 0;

    virtual void close() = 0;

    // Write raw bytes (the fully formed VISCA packet).
    // Returns true if all bytes were written.
    virtual bool write(const std::vector<uint8_t>& data) = 0;

    // Clear any pending input data in the receive buffer.
    virtual void purgeInput() = 0;

    // Read one complete VISCA response packet (bytes until and including 0xFF).
    // Returns the packet on success, or empty vector on timeout or error.
    // Uses an overall timeout.
    virtual std::vector<uint8_t> readPacket(int timeoutMs = 1000) = 0;
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_SERIAL_TRANSPORT_H
