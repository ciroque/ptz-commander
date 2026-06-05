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

    // Optional: could add read() later for ACK/completion/inquiry responses.
    // For initial implementation we are fire-and-forget on the command path.
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_SERIAL_TRANSPORT_H
