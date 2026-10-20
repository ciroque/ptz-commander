#ifndef CAMERAS_VISCA_VISCA_CALIBRATION_H
#define CAMERAS_VISCA_VISCA_CALIBRATION_H

#include "ViscaProfile.h"
#include "ViscaTransport.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace cameras::visca {

// The camera reports raw stops but not angles, so the degree span comes from
// the spec sheet. Pan is split evenly around home; tilt is down/up of home.
struct CalibrationRanges {
    float panSpanDeg = 340.0f;
    float tiltDownDeg = 90.0f;
    float tiltUpDeg = 90.0f;
};

struct RawPosition {
    uint16_t pan = 0;
    uint16_t tilt = 0;
    uint16_t zoom = 0;
};

// Measures a camera's ranges by sending it home, then driving pan, tilt and
// zoom to their stops in each direction and reading the positions. Every
// command's reply is checked (buffer-full is retried) and every phase must
// show the camera moving. The camera is sent home with zoom fully wide
// whether or not calibration succeeds; if it cannot be confirmed there, the
// error (or a warning, on success) says it may have been left mid-move.
class ViscaCalibrator {
public:
    using Progress = std::function<void(const std::string&)>;

    ViscaCalibrator(ViscaTransport& transport, uint8_t address, Progress progress = {});

    // On failure returns nullopt and sets error.
    std::optional<ViscaProfile> run(const CalibrationRanges& ranges, std::string& error);

    // Axes a phase must see move before a settle counts as a stop.
    enum Axis : unsigned { kNone = 0, kPan = 1, kTilt = 2, kZoom = 4 };

private:
    std::optional<RawPosition> readPosition();
    // Sends a command and reads its reply: completion accepts, buffer-full
    // retries, any other error fails. Silence is accepted; the caller's
    // movement check catches a command that was never received.
    bool sendCommand(const std::vector<uint8_t>& cmd, const char* what, std::string& error);
    bool stopDrives(std::string& error);
    // Waits until position holds still. With `from`, the axes in mustMove
    // must first move away from it, so a drive that never started is an
    // error rather than a stop.
    std::optional<RawPosition> waitForStop(const char* what, const std::optional<RawPosition>& from,
                                           unsigned mustMove, std::string& error);
    // Sends Home and zoom wide, and waits until the camera is at home
    // (and at wideZoom, when known).
    bool returnHome(const std::optional<RawPosition>& home, std::optional<uint16_t> wideZoom,
                    std::string& error);
    void report(const std::string& message) const;

    ViscaTransport& transport_;
    uint8_t address_ = 1;
    Progress progress_;
};

// Builds one axis from measured stops, choosing unsigned or signed raw
// positions so that home lies between the stops. Exposed for testing.
std::optional<ViscaAxis> axisFromStops(uint16_t minRaw, uint16_t homeRaw, uint16_t maxRaw,
                                       float minDeg, float maxDeg, std::string& error);

// A through-home axis needs degree width and raw travel on both sides of
// home; otherwise one side collapses and its angles map to the wrong place.
bool validAxis(const ViscaAxis& axis, std::string& error);

// validAxis for pan and tilt plus zoom travel. Applied to measured profiles
// and to calibrations read from setup files.
bool validCalibration(const ViscaProfile& profile, std::string& error);

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_CALIBRATION_H
