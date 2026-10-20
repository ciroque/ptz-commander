#ifndef CAMERAS_VISCA_VISCA_IP_ENDPOINT_H
#define CAMERAS_VISCA_VISCA_IP_ENDPOINT_H

#include "ViscaProfile.h"

#include <optional>
#include <string>

namespace cameras::visca {

struct ViscaIpEndpoint {
    std::string host;
    int port = 52381;
    std::string profile = "tenveo-30x";
    // Measured by `camera calibrate`; used instead of the named profile when set.
    std::optional<ViscaProfile> calibration;
};

// The profile a camera added from this endpoint would use: the calibration
// if present, else the named profile (empty means tenveo-30x). nullopt for an
// unknown name.
inline std::optional<ViscaProfile> resolveProfile(const ViscaIpEndpoint& endpoint) {
    if (endpoint.calibration) {
        return endpoint.calibration;
    }
    return profileByName(endpoint.profile.empty() ? "tenveo-30x" : endpoint.profile);
}

} // namespace cameras::visca

#endif
