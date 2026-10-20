#ifndef CAMERAS_VISCA_VISCA_PROFILE_H
#define CAMERAS_VISCA_VISCA_PROFILE_H

#include <cstdint>
#include <optional>
#include <string>

namespace cameras::visca {

// One pan or tilt axis: the degree range the tool exposes and the raw VISCA
// positions at its mechanical stops. minDeg/minRaw is left (pan) or down (tilt).
struct ViscaAxis {
    float minDeg = 0.0f;
    float maxDeg = 0.0f;
    uint16_t minRaw = 0x0000;
    uint16_t homeRaw = 0x8000;
    uint16_t maxRaw = 0xFFFF;
    // Map each side of home separately so 0 degrees is the camera's home even
    // when home is not the midpoint of the raw range.
    bool throughHome = false;
    // Raw positions are two's-complement (e.g. Sony: left of home is 0xF6xx).
    bool signedRaw = false;
};

struct ViscaProfile {
    std::string name = "lumens-vc-a51p";
    ViscaAxis pan{-170.0f, 170.0f, 0x0000, 0x8000, 0xFFFF, false};
    ViscaAxis tilt{-30.0f, 90.0f, 0x0000, 0x8000, 0xFFFF, false};
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
    p.pan = {-175.0f, 175.0f, 0x0000, 0x4A6C, 0x8E3E, true};
    // Spec tilt ±90°. Mechanical stops on 192.168.1.145 (absolute 0x0000 and
    // 0xFFFF, then inquiry). User convention: −90 down, 0 level, +90 up.
    // Higher raw is up (0x3C8A); raw 0 is down.
    p.tilt = {-90.0f, 90.0f, 0x0000, 0x8000, 0x3C8A, false};
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
