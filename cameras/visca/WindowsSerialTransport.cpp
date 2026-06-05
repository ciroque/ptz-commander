#include "WindowsSerialTransport.h"

#ifdef _WIN32

#include <windows.h>
#include <string>
#include <vector>

#include "../../core/Logger.h"

namespace cameras::visca {

WindowsSerialTransport::WindowsSerialTransport() = default;

WindowsSerialTransport::~WindowsSerialTransport() {
    close();
}

bool WindowsSerialTransport::open(const std::string& port, int baud) {
    close();  // ensure clean state

    // Windows serial ports are addressed as \\.\COMn to support > COM9
    std::string target = "\\\\.\\" + port;
    portName_ = port;

    handle_ = CreateFileA(target.c_str(),
                          GENERIC_READ | GENERIC_WRITE,
                          0,                // exclusive
                          nullptr,
                          OPEN_EXISTING,
                          0,                // no overlapped for simplicity
                          nullptr);

    if (handle_ == INVALID_HANDLE_VALUE) {
        core::Logger::error("Failed to open serial port " + port + " (error " + std::to_string(GetLastError()) + ")");
        return false;
    }

    // Configure DCB
    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(handle_, &dcb)) {
        core::Logger::error("GetCommState failed for " + port);
        close();
        return false;
    }

    dcb.BaudRate = static_cast<DWORD>(baud);
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fBinary = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX  = FALSE;

    if (!SetCommState(handle_, &dcb)) {
        core::Logger::error("SetCommState failed for " + port + " @ " + std::to_string(baud));
        close();
        return false;
    }

    // Set timeouts (read not critical yet; write should be reasonably quick)
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 100;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    timeouts.WriteTotalTimeoutConstant   = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(handle_, &timeouts)) {
        core::Logger::warn("SetCommTimeouts had issues on " + port);
        // non-fatal for our use case
    }

    // Purge any stale data
    PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);

    core::Logger::info("Opened VISCA serial port " + port + " @ " + std::to_string(baud) + " 8N1");
    return true;
}

bool WindowsSerialTransport::isOpen() const {
    return handle_ != INVALID_HANDLE_VALUE;
}

void WindowsSerialTransport::close() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        core::Logger::info("Closed serial port " + portName_);
    }
}

bool WindowsSerialTransport::write(const std::vector<uint8_t>& data) {
    if (!isOpen() || data.empty()) {
        return false;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(handle_, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);

    if (!ok || written != data.size()) {
        core::Logger::error("Serial write incomplete or failed on " + portName_ +
                            " (wrote " + std::to_string(written) + "/" + std::to_string(data.size()) + ")");
        return false;
    }

    return true;
}

void WindowsSerialTransport::purgeInput() {
    if (isOpen()) {
        PurgeComm(handle_, PURGE_RXABORT | PURGE_RXCLEAR);
    }
}

std::vector<uint8_t> WindowsSerialTransport::readPacket(int timeoutMs) {
    if (!isOpen()) {
        return {};
    }

    // Save and temporarily adjust timeouts for the read
    COMMTIMEOUTS oldTimeouts = {};
    GetCommTimeouts(handle_, &oldTimeouts);

    COMMTIMEOUTS to = oldTimeouts;
    to.ReadIntervalTimeout = 30;                 // allow small gaps between bytes
    to.ReadTotalTimeoutConstant = timeoutMs;
    to.ReadTotalTimeoutMultiplier = 0;
    SetCommTimeouts(handle_, &to);

    std::vector<uint8_t> packet;
    uint8_t byte = 0;
    DWORD bytesRead = 0;
    DWORD startTime = GetTickCount();

    while (GetTickCount() - startTime < static_cast<DWORD>(timeoutMs) + 50) {
        if (ReadFile(handle_, &byte, 1, &bytesRead, nullptr) && bytesRead == 1) {
            packet.push_back(byte);
            if (byte == 0xFF) {
                // restore timeouts
                SetCommTimeouts(handle_, &oldTimeouts);
                return packet;
            }
            if (packet.size() > 64) {
                break; // safety, malformed
            }
        } else {
            // no data or error; small sleep to yield
            Sleep(1);
        }
    }

    SetCommTimeouts(handle_, &oldTimeouts);

    if (!packet.empty()) {
        core::Logger::debug("readPacket timeout or incomplete on " + portName_ +
                            " (got " + std::to_string(packet.size()) + " bytes)");
    }
    return {};  // timeout or error → empty (caller treats as failure)
}

} // namespace cameras::visca

#endif // _WIN32
