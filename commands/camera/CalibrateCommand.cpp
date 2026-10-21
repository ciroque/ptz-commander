#include "CalibrateCommand.h"
#include "../Arguments.h"
#include "../ResolveCameras.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace commands::camera {
    namespace {
        std::optional<float> parseDegrees(const std::string& token, float min, float max) {
            try {
                size_t used = 0;
                const float value = std::stof(token, &used);
                // NaN fails no comparison, so check finiteness first.
                if (used != token.size() || !std::isfinite(value) || value < min || value > max) {
                    return std::nullopt;
                }
                return value;
            }
            catch (const std::exception&) {
                return std::nullopt;
            }
        }

        // Format into a local stream so ctx.out keeps whatever state the
        // other commands' tables rely on.
        std::string hex(uint16_t value) {
            std::ostringstream ss;
            ss << "0x" << std::hex << std::uppercase << std::right
               << std::setfill('0') << std::setw(4) << value;
            return ss.str();
        }

        std::string describeAxis(const cameras::visca::ViscaAxis& axis) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << axis.minDeg << " to " << axis.maxDeg
               << " deg, raw " << hex(axis.minRaw) << " / " << hex(axis.homeRaw)
               << " / " << hex(axis.maxRaw) << (axis.signedRaw ? " (signed)" : "");
            return ss.str();
        }
    }

    void CalibrateCommand::execute(data::Context& ctx, const std::string& args) {
        const std::string usage = "Usage: camera calibrate <id> [panSpan] [tiltDown] [tiltUp]";
        auto tokens = commands::splitArgs(args);
        if (tokens.empty() || tokens.size() > 4) {
            ctx.err << usage << std::endl;
            return;
        }
        if (tokens[0] == "*") {
            ctx.err << "Calibrate one camera at a time." << std::endl;
            return;
        }
        if (!ctx.calibrateVisca) {
            ctx.err << "VISCA is not available." << std::endl;
            return;
        }

        cameras::visca::CalibrationRanges ranges;
        const struct {
            float* target;
            float min;
            float max;
            const char* name;
        } fields[] = {
            {&ranges.panSpanDeg, 1.0f, 360.0f, "panSpan (1-360)"},
            // Both tilt sides need width: 0 degrees is home and each side of
            // home is mapped separately.
            {&ranges.tiltDownDeg, 1.0f, 180.0f, "tiltDown (1-180)"},
            {&ranges.tiltUpDeg, 1.0f, 180.0f, "tiltUp (1-180)"},
        };
        for (size_t i = 1; i < tokens.size(); ++i) {
            const auto& field = fields[i - 1];
            const auto value = parseDegrees(tokens[i], field.min, field.max);
            if (!value) {
                ctx.err << "Invalid " << field.name << ": " << tokens[i] << std::endl;
                return;
            }
            *field.target = *value;
        }

        auto cameras = commands::resolveCameras(ctx, tokens[0]);
        if (cameras.empty()) {
            return;
        }
        const auto serial = cameras.front()->getSerialNumber();

        // The header waits for the first step so a camera rejected up front
        // (not VISCA) does not announce that it is about to move.
        bool started = false;
        std::string error;
        const auto profile = ctx.calibrateVisca(serial, ranges,
            [&ctx, &serial, &started](const std::string& step) {
                if (!started) {
                    ctx.out << "Calibrating " << serial
                            << ". The camera will move through its full range." << std::endl;
                    started = true;
                }
                ctx.out << "  " << step << std::endl;
            },
            error);
        if (!profile) {
            ctx.err << "Calibration failed: " << error << std::endl;
            return;
        }

        ctx.notifySetupChanged();
        ctx.out << "Calibrated " << serial << ":" << std::endl
                << "  pan   " << describeAxis(profile->pan) << std::endl
                << "  tilt  " << describeAxis(profile->tilt) << std::endl
                << "  zoom  raw " << hex(profile->zoomMinRaw) << " to " << hex(profile->zoomMaxRaw) << std::endl
                << "Existing presets were converted to keep their positions." << std::endl
                << "Run `setup save` to keep this calibration." << std::endl;
    }
}
