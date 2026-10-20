#ifndef CAMERAS_VISCA_VISCA_COMMANDS_H
#define CAMERAS_VISCA_VISCA_COMMANDS_H

#include "ViscaProfile.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace cameras::visca {

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
