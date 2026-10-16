#include "ViscaCommands.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace cameras::visca {

namespace {
    uint16_t mapRange(float value, float inMin, float inMax, uint16_t outMin, uint16_t outMax) {
        if (inMax == inMin) {
            return outMin;
        }
        value = std::clamp(value, inMin, inMax);
        const float norm = (value - inMin) / (inMax - inMin);
        const float span = static_cast<float>(outMax) - static_cast<float>(outMin);
        return static_cast<uint16_t>(outMin + norm * span + 0.5f);
    }

    float unmapRange(uint16_t value, uint16_t inMin, uint16_t inMax, float outMin, float outMax) {
        if (inMax == inMin) {
            return outMin;
        }
        if (value <= inMin) {
            return outMin;
        }
        if (value >= inMax) {
            return outMax;
        }
        const float norm = static_cast<float>(value - inMin) / static_cast<float>(inMax - inMin);
        return outMin + norm * (outMax - outMin);
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
    return mapRange(degrees, profile.panMinDeg, profile.panMaxDeg,
                    profile.panMinRaw, profile.panMaxRaw);
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
    return unmapRange(visca, profile.panMinRaw, profile.panMaxRaw,
                      profile.panMinDeg, profile.panMaxDeg);
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
