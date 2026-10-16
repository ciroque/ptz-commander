#include "ViscaCommands.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace cameras::visca {

namespace {
    uint16_t mapRange(float value, float inA, float inB, uint16_t outA, uint16_t outB) {
        if (inA == inB) {
            return outA;
        }
        const float lo = std::min(inA, inB);
        const float hi = std::max(inA, inB);
        const float cv = std::clamp(value, lo, hi);
        const float norm = (cv - inA) / (inB - inA);
        const float span = static_cast<float>(outB) - static_cast<float>(outA);
        return static_cast<uint16_t>(outA + norm * span + 0.5f);
    }

    float unmapRange(uint16_t value, uint16_t inA, uint16_t inB, float outA, float outB) {
        if (inA == inB) {
            return outA;
        }
        const float a = static_cast<float>(inA);
        const float b = static_cast<float>(inB);
        const float v = std::clamp(static_cast<float>(value), std::min(a, b), std::max(a, b));
        const float norm = (v - a) / (b - a);
        return outA + norm * (outB - outA);
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
    const uint16_t panPos = panDegreesToVisca(panDeg, profile);
    const uint16_t tiltPos = tiltDegreesToVisca(tiltDeg, profile);

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
    std::vector<uint8_t> cmd;
    cmd.push_back(addressByte(address));
    cmd.push_back(0x01);
    cmd.push_back(0x04);
    cmd.push_back(0x47);
    appendNibblePosition(cmd, zoomPercentToVisca(zoomPercent, profile));
    cmd.push_back(0xFF);
    return cmd;
}

std::vector<uint8_t> ViscaCommands::panTiltPositionInquiry(uint8_t address) {
    return {addressByte(address), 0x09, 0x06, 0x12, 0xFF};
}

std::vector<uint8_t> ViscaCommands::zoomPositionInquiry(uint8_t address) {
    return {addressByte(address), 0x09, 0x04, 0x47, 0xFF};
}

uint16_t ViscaCommands::panDegreesToVisca(float degrees, const ViscaProfile& profile) {
    if (!profile.panThroughHome) {
        return mapRange(degrees, profile.panMinDeg, profile.panMaxDeg,
                        profile.panMinRaw, profile.panMaxRaw);
    }
    const float d = std::clamp(degrees, profile.panMinDeg, profile.panMaxDeg);
    if (d <= 0.0f) {
        return mapRange(d, profile.panMinDeg, 0.0f,
                        profile.panMinRaw, profile.panHomeRaw);
    }
    return mapRange(d, 0.0f, profile.panMaxDeg,
                    profile.panHomeRaw, profile.panMaxRaw);
}

uint16_t ViscaCommands::tiltDegreesToVisca(float degrees, const ViscaProfile& profile) {
    return mapRange(degrees, profile.tiltMinDeg, profile.tiltMaxDeg,
                    profile.tiltMinRaw, profile.tiltMaxRaw);
}

uint16_t ViscaCommands::zoomPercentToVisca(int percent, const ViscaProfile& profile) {
    percent = std::clamp(percent, 0, 100);
    return mapRange(static_cast<float>(percent), 0.0f, 100.0f,
                    profile.zoomMinRaw, profile.zoomMaxRaw);
}

float ViscaCommands::viscaToPanDegrees(uint16_t visca, const ViscaProfile& profile) {
    if (!profile.panThroughHome) {
        return unmapRange(visca, profile.panMinRaw, profile.panMaxRaw,
                          profile.panMinDeg, profile.panMaxDeg);
    }
    const bool increasing = profile.panMinRaw <= profile.panMaxRaw;
    const bool onMinSide = increasing ? (visca <= profile.panHomeRaw)
                                      : (visca >= profile.panHomeRaw);
    if (onMinSide) {
        return unmapRange(visca, profile.panMinRaw, profile.panHomeRaw,
                          profile.panMinDeg, 0.0f);
    }
    return unmapRange(visca, profile.panHomeRaw, profile.panMaxRaw,
                      0.0f, profile.panMaxDeg);
}

float ViscaCommands::viscaToTiltDegrees(uint16_t visca, const ViscaProfile& profile) {
    return unmapRange(visca, profile.tiltMinRaw, profile.tiltMaxRaw,
                      profile.tiltMinDeg, profile.tiltMaxDeg);
}

int ViscaCommands::viscaToZoomPercent(uint16_t visca, const ViscaProfile& profile) {
    return static_cast<int>(unmapRange(visca, profile.zoomMinRaw, profile.zoomMaxRaw,
                                       0.0f, 100.0f) + 0.5f);
}

} // namespace cameras::visca
