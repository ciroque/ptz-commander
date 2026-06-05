#ifndef CAMERAS_VISCA_VISCA_COMMANDS_H
#define CAMERAS_VISCA_VISCA_COMMANDS_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>

namespace cameras::visca {

// VISCA uses 4-nibble (16-bit) encoding for positions:
// value 0xABCD is sent as bytes 0x0A, 0x0B, 0x0C, 0x0D

// Common defaults for generic VISCA serial PTZ cameras (e.g. Lumens VC series, Sony-style).
// These can be tuned or made per-camera/profile in the future without changing the wire format.
namespace defaults {
    constexpr float kPanMinDeg   = -170.0f;
    constexpr float kPanMaxDeg   =  170.0f;
    constexpr float kTiltMinDeg  =  -30.0f;   // typical conference PTZ; widen if needed for other models
    constexpr float kTiltMaxDeg  =   90.0f;
    constexpr uint16_t kZoomMin  = 0x0000;
    constexpr uint16_t kZoomMax  = 0x4000;    // typical max for ~12-20x optical zoom
}

// Build exact VISCA absolute command packets (address byte + ... + 0xFF terminator).
// Only absolute positioning commands are provided (per requirements; no velocity "drive" commands).

struct ViscaCommands {
    // Home (go to mechanical home / 0,0,0)
    static std::vector<uint8_t> home(uint8_t address = 1);

    // Absolute Pan/Tilt position.
    // pan/tilt in degrees (will be clamped to the default ranges above and mapped to VISCA 16-bit units).
    // speeds are VISCA 0x01 (slow) .. ~0x18 (fast); 0x10 is a reasonable default.
    static std::vector<uint8_t> panTiltAbsolute(float panDeg, float tiltDeg,
                                                uint8_t panSpeed = 0x10,
                                                uint8_t tiltSpeed = 0x10,
                                                uint8_t address = 1);

    // Absolute (direct) zoom position.
    // zoomPercent: 0 (wide) .. 100 (full tele)
    static std::vector<uint8_t> zoomAbsolute(int zoomPercent, uint8_t address = 1);

    // Conversion helpers (exposed for testing / future use; mapping is linear + clamp).
    static uint16_t panDegreesToVisca(float degrees);
    static uint16_t tiltDegreesToVisca(float degrees);
    static uint16_t zoomPercentToVisca(int percent);

private:
    static uint8_t addressByte(uint8_t address);
    static void appendNibblePosition(std::vector<uint8_t>& cmd, uint16_t value);
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_COMMANDS_H
