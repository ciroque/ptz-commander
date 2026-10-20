#include "ViscaCalibration.h"

#include "ViscaCommands.h"
#include "../../core/Logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <thread>

namespace cameras::visca {

namespace {
    using Clock = std::chrono::steady_clock;

    constexpr auto kPollInterval = std::chrono::milliseconds(150);
    // Ignore "not moving" while a drive is still spinning up.
    constexpr auto kSpinUp = std::chrono::milliseconds(600);
    constexpr auto kSettleTimeout = std::chrono::seconds(30);
    constexpr auto kReplyTimeout = std::chrono::milliseconds(500);
    // A drive that has not moved anything by then was not executed.
    constexpr auto kStartTimeout = std::chrono::seconds(2);
    // How long to listen for a command's completion or error.
    constexpr auto kAcceptWindow = std::chrono::milliseconds(400);
    // Buffer-full retries: 20 x 100 ms covers a command still finishing.
    constexpr auto kBufferFullRetry = std::chrono::milliseconds(100);
    constexpr int kBufferFullAttempts = 20;
    constexpr int kSettleReads = 3;
    constexpr int kSettleTolerance = 2;
    constexpr int kMinTravel = 16;
    // Home is an absolute move; it lands a few units off (measured 2-4).
    constexpr int kArriveTolerance = 16;

    // Distance on the 16-bit ring, so a signed axis hovering across home
    // (0xFFFF vs 0x0000) counts as one unit apart, not 65535.
    int rawDistance(uint16_t a, uint16_t b) {
        const int d = std::abs(static_cast<int>(a) - static_cast<int>(b));
        return std::min(d, 0x10000 - d);
    }

    bool withinTolerance(uint16_t a, uint16_t b) {
        return rawDistance(a, b) <= kSettleTolerance;
    }

    bool settled(const RawPosition& a, const RawPosition& b) {
        return withinTolerance(a.pan, b.pan) && withinTolerance(a.tilt, b.tilt) &&
               withinTolerance(a.zoom, b.zoom);
    }

    bool between(int lo, int mid, int hi) {
        return (lo <= mid && mid <= hi) || (hi <= mid && mid <= lo);
    }

    std::string hex(uint16_t v) {
        static const char* digits = "0123456789ABCDEF";
        std::string s = "0x0000";
        for (int i = 0; i < 4; ++i) {
            s[5 - i] = digits[(v >> (i * 4)) & 0xF];
        }
        return s;
    }

    std::string describe(const RawPosition& p) {
        return "pan=" + hex(p.pan) + " tilt=" + hex(p.tilt) + " zoom=" + hex(p.zoom);
    }

    std::string describeAxes(unsigned axes) {
        std::string s;
        const auto add = [&s](const char* name) { s += (s.empty() ? "" : ", ") + std::string(name); };
        if (axes & ViscaCalibrator::kPan) add("pan");
        if (axes & ViscaCalibrator::kTilt) add("tilt");
        if (axes & ViscaCalibrator::kZoom) add("zoom");
        return s;
    }

    // VISCA error replies are 9x 6y EE FF.
    std::string errorName(uint8_t code) {
        switch (code) {
            case 0x01: return "message length error";
            case 0x02: return "syntax error";
            case 0x03: return "command buffer full";
            case 0x04: return "command cancelled";
            case 0x05: return "no socket";
            case 0x41: return "command not executable";
            default: return "error 0x" + hex(code).substr(4);
        }
    }
}

std::optional<ViscaAxis> axisFromStops(uint16_t minRaw, uint16_t homeRaw, uint16_t maxRaw,
                                       float minDeg, float maxDeg, std::string& error) {
    const auto s = [](uint16_t v) { return static_cast<int>(static_cast<int16_t>(v)); };
    const bool signedRaw = !between(minRaw, homeRaw, maxRaw) &&
                           between(s(minRaw), s(homeRaw), s(maxRaw));
    const ViscaAxis axis{minDeg, maxDeg, minRaw, homeRaw, maxRaw, true, signedRaw};
    if (!validAxis(axis, error)) {
        return std::nullopt;
    }
    return axis;
}

bool validAxis(const ViscaAxis& axis, std::string& error) {
    if (!std::isfinite(axis.minDeg) || !std::isfinite(axis.maxDeg)) {
        error = "degree range is not a number";
        return false;
    }
    const auto lin = [&axis](uint16_t v) {
        return axis.signedRaw ? static_cast<int>(static_cast<int16_t>(v)) : static_cast<int>(v);
    };
    if (!axis.throughHome) {
        if (!(axis.minDeg < axis.maxDeg)) {
            error = "degree range is empty";
            return false;
        }
        if (std::abs(lin(axis.maxRaw) - lin(axis.minRaw)) < kMinTravel) {
            error = "has no travel between " + hex(axis.minRaw) + " and " + hex(axis.maxRaw);
            return false;
        }
        return true;
    }

    // Through-home axes map each side separately, so each side needs width
    // in both degrees and raw positions.
    if (!(axis.minDeg < 0.0f && axis.maxDeg > 0.0f)) {
        error = "degree range must extend both sides of 0";
        return false;
    }
    if (!between(lin(axis.minRaw), lin(axis.homeRaw), lin(axis.maxRaw))) {
        error = "home " + hex(axis.homeRaw) + " is not between the stops " +
                hex(axis.minRaw) + " and " + hex(axis.maxRaw);
        return false;
    }
    if (std::abs(lin(axis.homeRaw) - lin(axis.minRaw)) < kMinTravel) {
        error = "has no travel between the left/down stop " + hex(axis.minRaw) +
                " and home " + hex(axis.homeRaw);
        return false;
    }
    if (std::abs(lin(axis.maxRaw) - lin(axis.homeRaw)) < kMinTravel) {
        error = "has no travel between home " + hex(axis.homeRaw) +
                " and the right/up stop " + hex(axis.maxRaw);
        return false;
    }
    return true;
}

bool validCalibration(const ViscaProfile& profile, std::string& error) {
    std::string axisError;
    if (!validAxis(profile.pan, axisError)) {
        error = "pan " + axisError;
        return false;
    }
    if (!validAxis(profile.tilt, axisError)) {
        error = "tilt " + axisError;
        return false;
    }
    if (std::abs(static_cast<int>(profile.zoomMaxRaw) - static_cast<int>(profile.zoomMinRaw)) < kMinTravel) {
        error = "zoom has no travel between wide " + hex(profile.zoomMinRaw) +
                " and tele " + hex(profile.zoomMaxRaw);
        return false;
    }
    return true;
}

ViscaCalibrator::ViscaCalibrator(ViscaTransport& transport, uint8_t address, Progress progress)
    : transport_(transport), address_(address), progress_(std::move(progress)) {}

void ViscaCalibrator::report(const std::string& message) const {
    core::Logger::info("VISCA calibration: " + message);
    if (progress_) {
        progress_(message);
    }
}

std::optional<RawPosition> ViscaCalibrator::readPosition() {
    // Replies to drive commands (ACK/completion) can still be queued, so read
    // until the inquiry's own reply arrives instead of taking the next packet.
    const auto inquire = [this](const std::vector<uint8_t>& cmd, auto parse)
        -> decltype(parse(std::vector<uint8_t>{})) {
        transport_.purgeInput();
        if (!transport_.write(cmd)) {
            return std::nullopt;
        }
        const auto deadline = Clock::now() + kReplyTimeout;
        while (Clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - Clock::now());
            const auto reply = transport_.readPacket(static_cast<int>(remaining.count()));
            if (reply.empty()) {
                break;
            }
            if (auto parsed = parse(reply)) {
                return parsed;
            }
        }
        return std::nullopt;
    };

    const auto pt = inquire(ViscaCommands::panTiltPositionInquiry(address_),
                            ViscaCommands::parsePanTiltPosition);
    if (!pt) {
        return std::nullopt;
    }
    const auto zoom = inquire(ViscaCommands::zoomPositionInquiry(address_),
                              ViscaCommands::parseZoomPosition);
    if (!zoom) {
        return std::nullopt;
    }
    return RawPosition{pt->pan, pt->tilt, *zoom};
}

bool ViscaCalibrator::sendCommand(const std::vector<uint8_t>& cmd, const char* what, std::string& error) {
    for (int attempt = 1; attempt <= kBufferFullAttempts; ++attempt) {
        transport_.purgeInput();
        if (!transport_.write(cmd)) {
            error = std::string("could not send ") + what;
            return false;
        }

        bool bufferFull = false;
        const auto deadline = Clock::now() + kAcceptWindow;
        while (!bufferFull && Clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - Clock::now());
            const auto reply = transport_.readPacket(static_cast<int>(remaining.count()));
            if (reply.empty()) {
                break;
            }
            if (reply.size() < 3 || reply[0] != 0x90) {
                continue;
            }
            const uint8_t kind = reply[1] & 0xF0;
            if (kind == 0x50 && reply.size() == 3) {
                return true;  // completion
            }
            if (kind == 0x60) {
                const uint8_t code = reply[2];
                if (code == 0x03) {
                    bufferFull = true;
                    continue;
                }
                error = std::string("camera rejected ") + what + " (" + errorName(code) + ")";
                return false;
            }
            // ACK (serial transports pass it through) or a stale inquiry
            // reply: keep reading for the completion or an error.
        }
        if (!bufferFull) {
            // No completion or error in the window. Some cameras complete a
            // command only when its motion finishes, and the UDP transport
            // hides the ACK, so this is not a failure on its own.
            return true;
        }
        core::Logger::debug(std::string("VISCA calibration: buffer full on ") + what +
                            ", retry " + std::to_string(attempt));
        std::this_thread::sleep_for(kBufferFullRetry);
    }
    error = std::string("camera stayed busy and did not accept ") + what;
    return false;
}

bool ViscaCalibrator::stopDrives(std::string& error) {
    // Send both even if the first is refused: a refused pan/tilt stop must
    // not leave the zoom drive running.
    std::string zoomError;
    const bool panTilt = sendCommand(ViscaCommands::panTiltDrive(0, 0, 0x18, 0x14, address_),
                                     "pan/tilt stop", error);
    const bool zoom = sendCommand(ViscaCommands::zoomDrive(0, address_), "zoom stop", zoomError);
    if (panTilt && !zoom) {
        error = zoomError;
    }
    return panTilt && zoom;
}

std::optional<RawPosition> ViscaCalibrator::waitForStop(const char* what,
                                                        const std::optional<RawPosition>& from,
                                                        unsigned mustMove, std::string& error) {
    const auto start = Clock::now();
    std::optional<RawPosition> last;
    int stableReads = 0;
    unsigned moved = kNone;

    while (Clock::now() - start < kSettleTimeout) {
        std::this_thread::sleep_for(kPollInterval);
        const auto pos = readPosition();
        if (!pos) {
            continue;
        }
        core::Logger::debug(std::string("VISCA calibration ") + what + ": " + describe(*pos));
        if (from) {
            // Same threshold validAxis accepts as travel.
            if (rawDistance(pos->pan, from->pan) >= kMinTravel) moved |= kPan;
            if (rawDistance(pos->tilt, from->tilt) >= kMinTravel) moved |= kTilt;
            if (rawDistance(pos->zoom, from->zoom) >= kMinTravel) moved |= kZoom;
            // A driven axis covers kMinTravel within milliseconds, so one that
            // still has not moved was never driven (or is already at a stop,
            // which validAxis would reject anyway).
            if ((moved & mustMove) != mustMove && Clock::now() - start >= kStartTimeout) {
                error = std::string(what) + ": camera did not move " + describeAxes(mustMove & ~moved);
                return std::nullopt;
            }
        }
        const bool allMoved = (moved & mustMove) == mustMove;
        if (allMoved && Clock::now() - start >= kSpinUp && last && settled(*last, *pos)) {
            if (++stableReads >= kSettleReads) {
                return pos;
            }
        } else {
            stableReads = 0;
        }
        last = pos;
    }

    error = std::string(what) + " did not settle within " +
            std::to_string(kSettleTimeout.count()) + "s";
    return std::nullopt;
}

bool ViscaCalibrator::returnHome(const std::optional<RawPosition>& home,
                                 std::optional<uint16_t> wideZoom, std::string& error) {
    report("returning home, zooming wide");
    // Home only moves pan/tilt, so zoom is driven wide; the drive stops by
    // itself at the wide limit. Zoom-wide is sent even if Home is refused,
    // since Home would not have replaced a zoom drive that is still running.
    std::string zoomError;
    const bool homing = sendCommand(ViscaCommands::home(address_), "home", error);
    const bool zooming = sendCommand(ViscaCommands::zoomDrive(-1, address_), "zoom wide", zoomError);
    if (!homing || !zooming) {
        if (homing) {
            error = zoomError;
        }
        // Report only once whatever is still moving has come to rest.
        std::string restError;
        waitForStop("return home", std::nullopt, kNone, restError);
        return false;
    }

    if (!home) {
        // Home was never measured, so all that can be checked is that the
        // camera came to rest.
        return waitForStop("return home", std::nullopt, kNone, error).has_value();
    }

    const auto start = Clock::now();
    std::optional<uint16_t> lastZoom;
    int zoomStillReads = 0;
    while (Clock::now() - start < kSettleTimeout) {
        std::this_thread::sleep_for(kPollInterval);
        const auto pos = readPosition();
        if (!pos) {
            continue;
        }
        // Without a measured wide stop, zoom has arrived once it stops
        // changing; stopping it earlier would cut the zoom-out short. As in
        // waitForStop, stillness during spin-up does not count.
        const bool spunUp = Clock::now() - start >= kSpinUp;
        zoomStillReads = (spunUp && lastZoom && withinTolerance(*lastZoom, pos->zoom))
                             ? zoomStillReads + 1 : 0;
        lastZoom = pos->zoom;
        const bool zoomWide = wideZoom ? rawDistance(pos->zoom, *wideZoom) <= kArriveTolerance
                                       : zoomStillReads >= kSettleReads;
        const bool atHome = rawDistance(pos->pan, home->pan) <= kArriveTolerance &&
                            rawDistance(pos->tilt, home->tilt) <= kArriveTolerance && zoomWide;
        if (atHome) {
            std::string stopError;
            if (!sendCommand(ViscaCommands::zoomDrive(0, address_), "zoom stop", stopError)) {
                core::Logger::warn("VISCA calibration: " + stopError);
            }
            return true;
        }
    }
    error = "camera did not reach home within " + std::to_string(kSettleTimeout.count()) + "s";
    return false;
}

std::optional<ViscaProfile> ViscaCalibrator::run(const CalibrationRanges& ranges, std::string& error) {
    // Check the camera answers before moving it anywhere.
    if (!readPosition()) {
        error = "camera did not answer position inquiries";
        return std::nullopt;
    }

    std::optional<RawPosition> home;
    std::optional<RawPosition> low;
    const auto fail = [&](const std::string& message) -> std::optional<ViscaProfile> {
        error = message;
        // Try the return even if the stop is refused: Home replaces a drive.
        std::string stopError;
        if (!stopDrives(stopError)) {
            core::Logger::warn("VISCA calibration: " + stopError);
        }
        std::string returnError;
        const std::optional<uint16_t> wideZoom =
            low ? std::optional<uint16_t>(low->zoom) : std::nullopt;
        if (!returnHome(home, wideZoom, returnError)) {
            error += "; " + returnError + ", so the camera may have been left mid-move";
        }
        return std::nullopt;
    };

    report("homing");
    if (!sendCommand(ViscaCommands::home(address_), "home", error)) {
        return fail(error);
    }
    // The camera may already be at home, so nothing has to move here.
    home = waitForStop("home", std::nullopt, kNone, error);
    if (!home) {
        return fail(error);
    }

    report("driving to the left and down stops, zooming wide");
    if (!sendCommand(ViscaCommands::panTiltDrive(-1, -1, 0x18, 0x14, address_), "pan/tilt drive", error) ||
        !sendCommand(ViscaCommands::zoomDrive(-1, address_), "zoom wide", error)) {
        return fail(error);
    }
    // Zoom may already be wide. Pan and tilt start at home, which a valid
    // calibration needs to be away from both stops, so both must move.
    low = waitForStop("left/down/wide", home, kPan | kTilt, error);
    if (!low) {
        return fail(error);
    }
    if (!stopDrives(error)) {
        return fail(error);
    }

    report("driving to the right and up stops, zooming tele");
    if (!sendCommand(ViscaCommands::panTiltDrive(1, 1, 0x18, 0x14, address_), "pan/tilt drive", error) ||
        !sendCommand(ViscaCommands::zoomDrive(1, address_), "zoom tele", error)) {
        return fail(error);
    }
    const auto high = waitForStop("right/up/tele", low, kPan | kTilt | kZoom, error);
    if (!high) {
        return fail(error);
    }
    if (!stopDrives(error)) {
        return fail(error);
    }

    report("home " + describe(*home));
    report("low  " + describe(*low));
    report("high " + describe(*high));

    ViscaProfile profile;
    profile.name = "calibrated";

    const float halfPan = ranges.panSpanDeg / 2.0f;
    const auto pan = axisFromStops(low->pan, home->pan, high->pan, -halfPan, halfPan, error);
    if (!pan) {
        return fail("pan " + error);
    }
    const auto tilt = axisFromStops(low->tilt, home->tilt, high->tilt,
                                    -ranges.tiltDownDeg, ranges.tiltUpDeg, error);
    if (!tilt) {
        return fail("tilt " + error);
    }
    profile.pan = *pan;
    profile.tilt = *tilt;
    profile.zoomMinRaw = low->zoom;
    profile.zoomMaxRaw = high->zoom;
    if (!validCalibration(profile, error)) {
        return fail(error);
    }

    // The measurement is good even if the return is not confirmed, so that
    // is a warning rather than a failure.
    std::string returnError;
    if (!returnHome(home, low->zoom, returnError)) {
        report("warning: " + returnError + "; the camera may have been left mid-move");
    }
    return profile;
}

} // namespace cameras::visca
