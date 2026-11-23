#include "TourCommand.h"
#include "../Arguments.h"
#include "../ConnectionLabel.h"
#include "../ResolveCameras.h"
#include "../TourDelay.h"
#include "../TourRunner.h"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace commands::preset {

    void TourCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty() || tokens.size() > 2) {
            ctx.err << "Usage: preset tour <id|*> [seconds]" << std::endl;
            return;
        }

        double delaySec = commands::kDefaultTourDelaySec;
        if (tokens.size() == 2) {
            const auto parsed = commands::parseTourSeconds(tokens[1]);
            if (!parsed) {
                ctx.err << "Usage: preset tour <id|*> [seconds]" << std::endl;
                return;
            }
            delaySec = *parsed;
        }

        auto cameras = commands::resolveCameras(ctx, tokens[0]);
        if (cameras.empty()) {
            return;
        }

        // No runner: the delay-0 tests stay on this thread. The first-flag
        // stays outside the camera loop, so the wait includes the gap
        // between cameras.
        if (!ctx.tour) {
            bool first = true;
            for (auto& camera : cameras) {
                if (commands::refuseNeedsLogin(ctx, *camera)) {
                    continue;
                }
                auto presets = camera->getPresets();
                if (presets.empty()) {
                    ctx.err << "No presets found for " << camera->getSerialNumber() << std::endl;
                    continue;
                }

                for (const auto& preset : presets) {
                    if (!first && delaySec > 0.0) {
                        ctx.out << "Waiting " << delaySec << "s..." << std::endl;
                        std::this_thread::sleep_for(std::chrono::duration<double>(delaySec));
                    }
                    first = false;
                    if (!camera->setPosition(preset.ptz.pan, preset.ptz.tilt, preset.ptz.zoom)) {
                        ctx.err << "Failed to apply preset '" << preset.name << "' to " << camera->getSerialNumber()
                                << std::endl;
                    } else {
                        ctx.out << "Applied preset '" << preset.name << "' to " << camera->getSerialNumber()
                                << std::endl;
                    }
                }
            }
            return;
        }

        std::vector<commands::TourRunner::Step> steps;
        for (auto& camera : cameras) {
            if (commands::refuseNeedsLogin(ctx, *camera)) {
                continue;
            }
            auto presets = camera->getPresets();
            if (presets.empty()) {
                ctx.err << "No presets found for " << camera->getSerialNumber() << std::endl;
                continue;
            }

            for (const auto& preset : presets) {
                auto cam = camera;
                auto presetCopy = preset;
                steps.push_back([cam, presetCopy](data::Context& stepCtx) {
                    if (!cam->setPosition(presetCopy.ptz.pan, presetCopy.ptz.tilt, presetCopy.ptz.zoom)) {
                        stepCtx.err << "Failed to apply preset '" << presetCopy.name << "' to "
                                    << cam->getSerialNumber() << std::endl;
                    } else {
                        stepCtx.out << "Applied preset '" << presetCopy.name << "' to " << cam->getSerialNumber()
                                    << std::endl;
                    }
                });
            }
        }
        if (steps.empty()) {
            return;
        }

        const auto delay
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(delaySec));
        ctx.tour->start(
            "preset tour " + tokens[0], std::move(steps), delay,
            [delaySec](data::Context& stepCtx) { stepCtx.out << "Waiting " << delaySec << "s..." << std::endl; }, {},
            ctx);
    }

} // namespace commands::preset
