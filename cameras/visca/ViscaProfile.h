#ifndef CAMERAS_VISCA_VISCA_PROFILE_H
#define CAMERAS_VISCA_VISCA_PROFILE_H

#include <cstdint>
#include <string>

namespace cameras::visca {

struct ViscaProfile {
    const char* name = "lumens-vc-a51p";
    float panMinDeg = -170.0f;
    float panMaxDeg = 170.0f;
    float tiltMinDeg = -30.0f;
    float tiltMaxDeg = 90.0f;
    uint16_t panMinRaw = 0x0000;
    uint16_t panMaxRaw = 0xFFFF;
    uint16_t tiltMinRaw = 0x0000;
    uint16_t tiltMaxRaw = 0xFFFF;
    uint16_t zoomMinRaw = 0x0000;
    uint16_t zoomMaxRaw = 0x4000;
};

inline ViscaProfile lumensVcA51p() {
    ViscaProfile p;
    p.name = "lumens-vc-a51p";
    return p;
}

inline ViscaProfile tenveo30x() {
    ViscaProfile p;
    p.name = "tenveo-30x";
    // Degree limits are typical 30x conference PTZ. Raw pan/tilt use the same
    // linear 16-bit map as Lumens until a measured Tenveo map replaces them.
    // Zoom 0x0000 is wide (verified on the Tenveo at 192.168.1.145).
    p.panMinDeg = -170.0f;
    p.panMaxDeg = 170.0f;
    p.tiltMinDeg = -30.0f;
    p.tiltMaxDeg = 90.0f;
    return p;
}

inline ViscaProfile profileByName(const std::string& name) {
    if (name == "tenveo-30x" || name == "tenveo") {
        return tenveo30x();
    }
    return lumensVcA51p();
}

} // namespace cameras::visca

#endif
