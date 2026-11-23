#include "TourCommand.h"
#include "../Arguments.h"
#include "../ConnectionLabel.h"
#include "../ResolveCameras.h"
#include "../TourDelay.h"

#include <chrono>
#include <thread>

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
                    ctx.out << "Applied preset '" << preset.name << "' to " << camera->getSerialNumber() << std::endl;
                }
            }
        }
    }

} // namespace commands::preset
