#ifndef CAMERAS_VISCA_VISCA_DISCOVERY_H
#define CAMERAS_VISCA_VISCA_DISCOVERY_H

#include <cstdint>
#include <string>
#include <vector>

namespace cameras::visca {

struct ViscaPortCandidate {
    std::string port;           // e.g. "COM3"
    int baud = 9600;
    uint8_t address = 1;
    uint16_t vid = 0;
    uint16_t pid = 0;
    std::string description;    // friendly name or hardware info
    bool matchesKeyspan = false;
};

// Discovers *currently visible* serial ports suitable for VISCA auto-discovery.
// It enumerates whatever COM ports Windows currently reports (via registry + SetupAPI friendly names + QueryDosDevice).
// This means: the USB serial adapter (e.g. Keyspan) must be physically plugged in for the port to appear.
// Priority / filter: devices matching Keyspan USA-19H (VID 0x06CD / PID 0x0121) or whose friendly name contains "keyspan"/"usa-19".
// The actual camera (Lumens etc.) does not need to be powered on for port discovery, but commands will fail until it is.
// Manual / specific-port configuration is still possible in the future (ViscaCamera ctor accepts any port).

struct ViscaDiscovery {
    // Returns candidates sorted with best matches (Keyspan) first.
    static std::vector<ViscaPortCandidate> discover();

    // Attempt to open the serial port at the candidate's baud rate.
    // Returns true if the open succeeds. No VISCA inquiry is sent and no
    // response is validated (response reading / ACK handling is still deferred).
    // The caller's confidence comes from prior VID/PID or friendly-name
    // filtering (matchesKeyspan) plus a successful open. A future
    // implementation may send a non-motion inquiry and check for a VISCA-like
    // reply (starting with 0x90).
    static bool probePort(const ViscaPortCandidate& candidate);
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_DISCOVERY_H
