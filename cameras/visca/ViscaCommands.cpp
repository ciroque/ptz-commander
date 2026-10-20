#include "ViscaCommands.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cameras::visca {

namespace {
    // Raw outputs are linear positions (see linear()); the result is rounded
    // and wrapped back into the 16-bit field the camera expects.
    uint16_t mapRange(float value, float inA, float inB, float outA, float outB) {
        if (inA == inB) {
            return static_cast<uint16_t>(static_cast<int32_t>(std::floor(outA + 0.5f)));
        }
        const float lo = std::min(inA, inB);
        const float hi = std::max(inA, inB);
        const float cv = std::clamp(value, lo, hi);
        const float norm = (cv - inA) / (inB - inA);
        const float out = outA + norm * (outB - outA);
        return static_cast<uint16_t>(static_cast<int32_t>(std::floor(out + 0.5f)));
    }

    float unmapRange(float value, float inA, float inB, float outA, float outB) {
        if (inA == inB) {
            return outA;
        }
        const float v = std::clamp(value, std::min(inA, inB), std::max(inA, inB));
        const float norm = (v - inA) / (inB - inA);
        return outA + norm * (outB - outA);
    }

    // Position on a straight line: signed axes read 0xF670 as -2448 so a range
    // that crosses 0x0000 stays monotonic.
    float linear(uint16_t raw, const ViscaAxis& axis) {
        return axis.signedRaw ? static_cast<float>(static_cast<int16_t>(raw))
                              : static_cast<float>(raw);
    }

    uint16_t axisDegreesToVisca(float degrees, const ViscaAxis& axis) {
        const float minRaw = linear(axis.minRaw, axis);
        const float homeRaw = linear(axis.homeRaw, axis);
        const float maxRaw = linear(axis.maxRaw, axis);
        if (!axis.throughHome) {
            return mapRange(degrees, axis.minDeg, axis.maxDeg, minRaw, maxRaw);
        }
        const float d = std::clamp(degrees, axis.minDeg, axis.maxDeg);
        // 0 degrees is home by definition. Without this, a zero-width degree
        // side (minDeg == 0) would short-circuit to minRaw.
        if (d == 0.0f) {
            return axis.homeRaw;
        }
        if (d < 0.0f) {
            return mapRange(d, axis.minDeg, 0.0f, minRaw, homeRaw);
        }
        return mapRange(d, 0.0f, axis.maxDeg, homeRaw, maxRaw);
    }

    float viscaToAxisDegrees(uint16_t visca, const ViscaAxis& axis) {
        const float v = linear(visca, axis);
        const float minRaw = linear(axis.minRaw, axis);
        const float homeRaw = linear(axis.homeRaw, axis);
        const float maxRaw = linear(axis.maxRaw, axis);
        if (!axis.throughHome) {
            return unmapRange(v, minRaw, maxRaw, axis.minDeg, axis.maxDeg);
        }
        // Likewise home reads as 0 even when a stop coincides with it.
        if (visca == axis.homeRaw) {
            return 0.0f;
        }
        const bool increasing = minRaw <= maxRaw;
        const bool onMinSide = increasing ? (v <= homeRaw) : (v >= homeRaw);
        if (onMinSide) {
            return unmapRange(v, minRaw, homeRaw, axis.minDeg, 0.0f);
        }
        return unmapRange(v, homeRaw, maxRaw, 0.0f, axis.maxDeg);
    }
}

uint8_t ViscaCommands::addressByte(uint8_t address) {
    if (address < 1) address = 1;
    if (address > 7) address = 7;
    return static_cast<uint8_t>(0x80 | address);
}

void ViscaCommands::appendNibblePosition(std::vector<uint8_t>& cmd, uint16_t pos) {
    cmd.push_back(static_cast<uint8_t>((pos >> 12) & 0x0F));
    cmd.push_back(static_cast<uint8_t>((pos >> 8) & 0x0F));
    cmd.push_back(static_cast<uint8_t>((pos >> 4) & 0x0F));
    cmd.push_back(static_cast<uint8_t>(pos & 0x0F));
}

std::vector<uint8_t> ViscaCommands::home(uint8_t address) {
    return {addressByte(address), 0x01, 0x06, 0x04, 0xFF};
}

std::vector<uint8_t> ViscaCommands::panTiltAbsolute(float panDeg, float tiltDeg,
                                                    const ViscaProfile& profile,
                                                    uint8_t panSpeed, uint8_t tiltSpeed,
                                                    uint8_t address) {
    return panTiltAbsoluteRaw(panDegreesToVisca(panDeg, profile),
                              tiltDegreesToVisca(tiltDeg, profile),
                              panSpeed, tiltSpeed, address);
}

std::vector<uint8_t> ViscaCommands::panTiltAbsoluteRaw(uint16_t panPos, uint16_t tiltPos,
                                                       uint8_t panSpeed, uint8_t tiltSpeed,
                                                       uint8_t address) {
    if (panSpeed < 0x01) panSpeed = 0x01;
    if (panSpeed > 0x18) panSpeed = 0x18;
    if (tiltSpeed < 0x01) tiltSpeed = 0x01;
    if (tiltSpeed > 0x18) tiltSpeed = 0x18;

    std::vector<uint8_t> cmd;
    cmd.push_back(addressByte(address));
    cmd.push_back(0x01);
    cmd.push_back(0x06);
    cmd.push_back(0x02);
    cmd.push_back(panSpeed);
    cmd.push_back(tiltSpeed);
    appendNibblePosition(cmd, panPos);
    appendNibblePosition(cmd, tiltPos);
    cmd.push_back(0xFF);
    return cmd;
}

std::vector<uint8_t> ViscaCommands::zoomAbsolute(int zoomPercent,
                                                 const ViscaProfile& profile,
                                                 uint8_t address) {
    return zoomAbsoluteRaw(zoomPercentToVisca(zoomPercent, profile), address);
}

std::vector<uint8_t> ViscaCommands::zoomAbsoluteRaw(uint16_t zoomPos, uint8_t address) {
    std::vector<uint8_t> cmd;
    cmd.push_back(addressByte(address));
    cmd.push_back(0x01);
    cmd.push_back(0x04);
    cmd.push_back(0x47);
    appendNibblePosition(cmd, zoomPos);
    cmd.push_back(0xFF);
    return cmd;
}

std::vector<uint8_t> ViscaCommands::panTiltPositionInquiry(uint8_t address) {
    return {addressByte(address), 0x09, 0x06, 0x12, 0xFF};
}

std::vector<uint8_t> ViscaCommands::zoomPositionInquiry(uint8_t address) {
    return {addressByte(address), 0x09, 0x04, 0x47, 0xFF};
}

namespace {
    uint16_t nibbles(const std::vector<uint8_t>& reply, size_t at) {
        return static_cast<uint16_t>((reply[at] << 12) | (reply[at + 1] << 8) |
                                     (reply[at + 2] << 4) | reply[at + 3]);
    }
}

std::optional<PanTiltRaw> ViscaCommands::parsePanTiltPosition(const std::vector<uint8_t>& reply) {
    if (reply.size() != 11 || reply.back() != 0xFF || reply[0] != 0x90 || reply[1] != 0x50) {
        return std::nullopt;
    }
    return PanTiltRaw{nibbles(reply, 2), nibbles(reply, 6)};
}

std::optional<uint16_t> ViscaCommands::parseZoomPosition(const std::vector<uint8_t>& reply) {
    if (reply.size() != 7 || reply.back() != 0xFF || reply[0] != 0x90 || reply[1] != 0x50) {
        return std::nullopt;
    }
    return nibbles(reply, 2);
}

uint16_t ViscaCommands::panDegreesToVisca(float degrees, const ViscaProfile& profile) {
    return axisDegreesToVisca(degrees, profile.pan);
}

uint16_t ViscaCommands::tiltDegreesToVisca(float degrees, const ViscaProfile& profile) {
    return axisDegreesToVisca(degrees, profile.tilt);
}

uint16_t ViscaCommands::zoomPercentToVisca(int percent, const ViscaProfile& profile) {
    percent = std::clamp(percent, 0, 100);
    return mapRange(static_cast<float>(percent), 0.0f, 100.0f,
                    profile.zoomMinRaw, profile.zoomMaxRaw);
}

float ViscaCommands::viscaToPanDegrees(uint16_t visca, const ViscaProfile& profile) {
    return viscaToAxisDegrees(visca, profile.pan);
}

float ViscaCommands::viscaToTiltDegrees(uint16_t visca, const ViscaProfile& profile) {
    return viscaToAxisDegrees(visca, profile.tilt);
}

int ViscaCommands::viscaToZoomPercent(uint16_t visca, const ViscaProfile& profile) {
    return static_cast<int>(unmapRange(visca, profile.zoomMinRaw, profile.zoomMaxRaw,
                                       0.0f, 100.0f) + 0.5f);
}

} // namespace cameras::visca
