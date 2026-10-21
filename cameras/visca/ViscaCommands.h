#ifndef CAMERAS_VISCA_VISCA_COMMANDS_H
#define CAMERAS_VISCA_VISCA_COMMANDS_H

#include "ViscaProfile.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace cameras::visca {

// The first byte of a reply from camera address 1-7: 0x90-0xF0.
inline uint8_t replyHeader(uint8_t address) { return static_cast<uint8_t>(0x80 | ((address & 0x07) << 4)); }
inline bool isReplyHeader(uint8_t byte) { return byte >= 0x90 && (byte & 0x0F) == 0; }

// Any reply from a camera, error replies included: header, at least one
// byte, FF.
inline bool isViscaReply(const std::vector<uint8_t>& packet) {
    return packet.size() >= 3 && isReplyHeader(packet[0]) && packet.back() == 0xFF;
}

struct PanTiltRaw {
    uint16_t pan = 0;
    uint16_t tilt = 0;
};

struct ViscaCommands {
    static std::vector<uint8_t> home(uint8_t address = 1);

    static std::vector<uint8_t> panTiltAbsolute(float panDeg, float tiltDeg,
                                                const ViscaProfile& profile,
                                                uint8_t panSpeed = 0x10,
                                                uint8_t tiltSpeed = 0x10,
                                                uint8_t address = 1);
    static std::vector<uint8_t> panTiltAbsoluteRaw(uint16_t panPos, uint16_t tiltPos,
                                                   uint8_t panSpeed = 0x10,
                                                   uint8_t tiltSpeed = 0x10,
                                                   uint8_t address = 1);

    static std::vector<uint8_t> zoomAbsolute(int zoomPercent,
                                             const ViscaProfile& profile,
                                             uint8_t address = 1);
    static std::vector<uint8_t> zoomAbsoluteRaw(uint16_t zoomPos, uint8_t address = 1);

    // Continuous drives until stopped. Direction: -1 left/down/wide, +1 right/up/tele, 0 stop.
    static std::vector<uint8_t> panTiltDrive(int panDir, int tiltDir,
                                             uint8_t panSpeed = 0x18,
                                             uint8_t tiltSpeed = 0x14,
                                             uint8_t address = 1);
    static std::vector<uint8_t> zoomDrive(int dir, uint8_t address = 1);

    static std::vector<uint8_t> panTiltPositionInquiry(uint8_t address = 1);
    static std::vector<uint8_t> zoomPositionInquiry(uint8_t address = 1);

    // Inquiry replies (90 50 ... FF). nullopt for anything else, e.g. a stale
    // completion or an error packet.
    static std::optional<PanTiltRaw> parsePanTiltPosition(const std::vector<uint8_t>& reply);
    static std::optional<uint16_t> parseZoomPosition(const std::vector<uint8_t>& reply);

    static uint16_t panDegreesToVisca(float degrees, const ViscaProfile& profile);
    static uint16_t tiltDegreesToVisca(float degrees, const ViscaProfile& profile);
    static uint16_t zoomPercentToVisca(int percent, const ViscaProfile& profile);

    static float viscaToPanDegrees(uint16_t visca, const ViscaProfile& profile);
    static float viscaToTiltDegrees(uint16_t visca, const ViscaProfile& profile);
    static int viscaToZoomPercent(uint16_t visca, const ViscaProfile& profile);

private:
    static uint8_t addressByte(uint8_t address);
    static void appendNibblePosition(std::vector<uint8_t>& cmd, uint16_t value);
};

} // namespace cameras::visca

#endif
