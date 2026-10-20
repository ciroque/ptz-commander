#ifndef CAMERAS_VISCA_VISCA_PROFILE_H
#define CAMERAS_VISCA_VISCA_PROFILE_H

#include <cstdint>
#include <optional>
#include <string>

namespace cameras::visca {

struct ViscaProfile {
    const char* name = "lumens-vc-a51p";
    float panMinDeg = -170.0f;
    float panMaxDeg = 170.0f;
    float tiltMinDeg = -30.0f;
    float tiltMaxDeg = 90.0f;
    uint16_t panMinRaw = 0x0000;
    uint16_t panHomeRaw = 0x8000;
    uint16_t panMaxRaw = 0xFFFF;
    bool panThroughHome = false;
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
    // Spec pan ±175° (350° total). Measured on 192.168.1.145: home 0x4A6C,
    // full left 0x0000, full right 0x8E3E. 0° is home, negative is left.
    p.panMinDeg = -175.0f;
    p.panMaxDeg = 175.0f;
    p.panMinRaw = 0x0000;
    p.panHomeRaw = 0x4A6C;
    p.panMaxRaw = 0x8E3E;
    p.panThroughHome = true;
    // Spec tilt ±90°. Mechanical stops on 192.168.1.145 (absolute 0x0000 and
    // 0xFFFF, then inquiry). User convention: −90 down, 0 level, +90 up.
    // Higher raw is up (0x3C8A); raw 0 is down.
    p.tiltMinDeg = -90.0f;
    p.tiltMaxDeg = 90.0f;
    p.tiltMinRaw = 0x0000;
    p.tiltMaxRaw = 0x3C8A;
    // Measured zoom: full out 0x0000, full in 0x05E3. Linear 0–100.
    p.zoomMinRaw = 0x0000;
    p.zoomMaxRaw = 0x05E3;
    return p;
}

// Unknown names return nullopt rather than a default, so a typo cannot
// silently put a camera on the wrong ranges.
inline std::optional<ViscaProfile> profileByName(const std::string& name) {
    if (name == "tenveo-30x" || name == "tenveo") {
        return tenveo30x();
    }
    if (name == "lumens-vc-a51p" || name == "lumens") {
        return lumensVcA51p();
    }
    return std::nullopt;
}

inline const char* knownProfileNames() {
    return "tenveo-30x, lumens-vc-a51p";
}

} // namespace cameras::visca

#endif
