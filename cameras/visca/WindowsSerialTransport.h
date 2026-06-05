#ifndef CAMERAS_VISCA_WINDOWS_SERIAL_TRANSPORT_H
#define CAMERAS_VISCA_WINDOWS_SERIAL_TRANSPORT_H

#include "SerialTransport.h"

#ifdef _WIN32

#include <windows.h>
#include <string>
#include <vector>

namespace cameras::visca {

class WindowsSerialTransport : public SerialTransport {
public:
    WindowsSerialTransport();
    ~WindowsSerialTransport() override;

    bool open(const std::string& port, int baud) override;
    bool isOpen() const override;
    void close() override;
    bool write(const std::vector<uint8_t>& data) override;

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::string portName_;  // for logging / diagnostics, e.g. "COM3"
};

} // namespace cameras::visca

#endif // _WIN32

#endif // CAMERAS_VISCA_WINDOWS_SERIAL_TRANSPORT_H
