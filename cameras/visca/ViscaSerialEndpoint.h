#ifndef CAMERAS_VISCA_VISCA_SERIAL_ENDPOINT_H
#define CAMERAS_VISCA_VISCA_SERIAL_ENDPOINT_H

#include "ViscaProfile.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace cameras::visca {

constexpr const char* kSerialDefaultProfile = "lumens-vc-a51p";
constexpr int kSerialDefaultBaud = 9600;

// A VISCA camera on a COM port. One camera per port: each camera opens its
// port exclusively, so cameras daisy-chained on one port aren't supported.
struct ViscaSerialEndpoint {
    std::string port;          // "COM3"
    int baud = kSerialDefaultBaud;
    int address = 1;           // VISCA camera address, 1-7
    std::string profile = kSerialDefaultProfile;
    // Measured by `camera calibrate`; used instead of the named profile when set.
    std::optional<ViscaProfile> calibration;
};

// "VISCA:COM3:1": the camera's serial number, and its key in setup files.
inline std::string serialCameraId(const ViscaSerialEndpoint& endpoint) {
    return "VISCA:" + endpoint.port + ":" + std::to_string(endpoint.address);
}

// "com3" or "COM3" as "COM3"; nullopt for anything but COM1-COM256.
inline std::optional<std::string> normalizeComPort(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (text.size() < 4 || text.size() > 6 || text.compare(0, 3, "COM") != 0 || text[3] == '0' ||
        !std::all_of(text.begin() + 3, text.end(), [](unsigned char c) { return std::isdigit(c); })) {
        return std::nullopt;
    }
    const int number = std::stoi(text.substr(3));
    if (number > 256) {
        return std::nullopt;
    }
    return text;
}

// The port and address in a serial camera id such as "VISCA:COM3:1";
// nullopt if id isn't one.
inline std::optional<ViscaSerialEndpoint> parseSerialCameraId(const std::string& id) {
    const std::string prefix = "VISCA:";
    const auto colon = id.rfind(':');
    if (id.compare(0, prefix.size(), prefix) != 0 || colon == std::string::npos || colon < prefix.size() ||
        colon + 2 != id.size() || id[colon + 1] < '1' || id[colon + 1] > '7') {
        return std::nullopt;
    }
    const std::string port = id.substr(prefix.size(), colon - prefix.size());
    if (normalizeComPort(port) != port) {
        return std::nullopt;
    }
    ViscaSerialEndpoint endpoint;
    endpoint.port = port;
    endpoint.address = id[colon + 1] - '0';
    return endpoint;
}

inline bool validSerialBaud(int baud) {
    return baud == 1200 || baud == 2400 || baud == 4800 || baud == 9600 || baud == 19200 ||
           baud == 38400 || baud == 57600 || baud == 115200;
}

inline const char* validSerialBauds() {
    return "1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200";
}

// The profile a camera added from this endpoint would use: the calibration
// if present, else the named profile (empty means lumens-vc-a51p). nullopt
// for an unknown name.
inline std::optional<ViscaProfile> resolveProfile(const ViscaSerialEndpoint& endpoint) {
    if (endpoint.calibration) {
        return endpoint.calibration;
    }
    return profileByName(endpoint.profile.empty() ? kSerialDefaultProfile : endpoint.profile);
}

} // namespace cameras::visca

#endif
