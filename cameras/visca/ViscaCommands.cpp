#include "ViscaCommands.h"

#include <cstdint>
#include <vector>

namespace cameras::visca {

namespace {

uint8_t addrByte(uint8_t address) {
    // VISCA camera address is 1-7 typically; command byte is 0x80 | address
    if (address < 1) address = 1;
    if (address > 7) address = 7;
    return static_cast<uint8_t>(0x80 | address);
}

void appendPosition(std::vector<uint8_t>& cmd, uint16_t pos) {
    // Split 16-bit into 4 nibbles, high nibble first
    cmd.push_back(static_cast<uint8_t>((pos >> 12) & 0x0F));
    cmd.push_back(static_cast<uint8_t>((pos >> 8)  & 0x0F));
    cmd.push_back(static_cast<uint8_t>((pos >> 4)  & 0x0F));
    cmd.push_back(static_cast<uint8_t>( pos        & 0x0F));
}

} // anonymous namespace

std::vector<uint8_t> ViscaCommands::home(uint8_t address) {
    std::vector<uint8_t> cmd;
    cmd.push_back(addrByte(address));
    cmd.push_back(0x01);
    cmd.push_back(0x06);
    cmd.push_back(0x04);
    cmd.push_back(0xFF);
    return cmd;
}

std::vector<uint8_t> ViscaCommands::panTiltAbsolute(float panDeg, float tiltDeg,
                                                    uint8_t panSpeed, uint8_t tiltSpeed,
                                                    uint8_t address) {
    uint16_t panPos  = panDegreesToVisca(panDeg);
    uint16_t tiltPos = tiltDegreesToVisca(tiltDeg);

    // Clamp speeds to reasonable VISCA range
    if (panSpeed  < 0x01) panSpeed  = 0x01;
    if (panSpeed  > 0x18) panSpeed  = 0x18;
    if (tiltSpeed < 0x01) tiltSpeed = 0x01;
    if (tiltSpeed > 0x18) tiltSpeed = 0x18;

    std::vector<uint8_t> cmd;
    cmd.push_back(addrByte(address));
    cmd.push_back(0x01);
    cmd.push_back(0x06);
    cmd.push_back(0x02);
    cmd.push_back(panSpeed);
    cmd.push_back(tiltSpeed);
    appendPosition(cmd, panPos);
    appendPosition(cmd, tiltPos);
    cmd.push_back(0xFF);
    return cmd;
}

std::vector<uint8_t> ViscaCommands::zoomAbsolute(int zoomPercent, uint8_t address) {
    uint16_t zoomPos = zoomPercentToVisca(zoomPercent);

    std::vector<uint8_t> cmd;
    cmd.push_back(addrByte(address));
    cmd.push_back(0x01);
    cmd.push_back(0x04);
    cmd.push_back(0x47);
    appendPosition(cmd, zoomPos);
    cmd.push_back(0xFF);
    return cmd;
}

uint16_t ViscaCommands::panDegreesToVisca(float degrees) {
    using namespace defaults;
    degrees = std::clamp(degrees, kPanMinDeg, kPanMaxDeg);
    float norm = (degrees - kPanMinDeg) / (kPanMaxDeg - kPanMinDeg);
    // 0.0 -> 0x0000 (left), 1.0 -> 0xFFFF (right), center ~0x8000
    return static_cast<uint16_t>(norm * 0xFFFFu + 0.5f);
}

uint16_t ViscaCommands::tiltDegreesToVisca(float degrees) {
    using namespace defaults;
    degrees = std::clamp(degrees, kTiltMinDeg, kTiltMaxDeg);
    float norm = (degrees - kTiltMinDeg) / (kTiltMaxDeg - kTiltMinDeg);
    return static_cast<uint16_t>(norm * 0xFFFFu + 0.5f);
}

uint16_t ViscaCommands::zoomPercentToVisca(int percent) {
    using namespace defaults;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    float norm = percent / 100.0f;
    return static_cast<uint16_t>(kZoomMin + norm * (kZoomMax - kZoomMin) + 0.5f);
}

} // namespace cameras::visca
