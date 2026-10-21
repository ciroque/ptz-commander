#ifndef CAMERAS_VISCA_WINDOWS_SERIAL_TRANSPORT_H
#define CAMERAS_VISCA_WINDOWS_SERIAL_TRANSPORT_H

#include "ViscaTransport.h"

#ifdef _WIN32

#include <windows.h>
#include <string>
#include <vector>

namespace cameras::visca {

class WindowsSerialTransport : public ViscaTransport {
public:
    // quiet: don't log a failed open (a scan reports it instead).
    WindowsSerialTransport(std::string port, int baud, bool quiet = false);
    ~WindowsSerialTransport() override;

    // The Windows error from the last failed open(), e.g. ERROR_ACCESS_DENIED
    // when another program has the port.
    DWORD lastError() const { return lastError_; }

    bool open() override;
    bool isOpen() const override;
    void close() override;
    bool write(const std::vector<uint8_t>& data) override;
    void purgeInput() override;
    std::vector<uint8_t> readPacket(int timeoutMs = 1000) override;

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::string portName_;
    int baud_ = 9600;
    bool quiet_ = false;
    DWORD lastError_ = 0;
};

} // namespace cameras::visca

#endif // _WIN32

#endif
