#include "ApplyCommand.h"
#include "../ConnectionLabel.h"
#include "../ResolveCameras.h"
#include "../TourRunner.h"

namespace commands::preset {
    namespace {
        void printUsage(data::Context& ctx) { ctx.err << "Usage: preset apply <id|*> <name>" << std::endl; }
    } // namespace

    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            printUsage(ctx);
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            printUsage(ctx);
            return;
        }

        const std::string& serialNumber = tokens[0];
        const std::string& presetName = tokens[1];
        // A star here used to cycle every preset. preset tour does that.
        if (presetName == "*") {
            ctx.err << "preset apply takes one preset name. Use: preset tour <id|*> [seconds]" << std::endl;
            return;
        }
        if (tokens.size() > 2) {
            printUsage(ctx);
            return;
        }

        if (ctx.tour) {
            ctx.tour->stop();
        }

        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        bool allGood = true;
        for (auto& camera : cameras) {
            if (commands::refuseNeedsLogin(ctx, *camera)) {
                allGood = false;
                continue;
            }

            const cameras::Preset* preset = camera->getPresetByName(presetName);
            if (!preset) {
                ctx.err << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << std::endl;
                allGood = false;
                continue;
            }

            if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                ctx.err << "Failed to apply preset '" << presetName << "' to " << camera->getSerialNumber()
                        << std::endl;
                allGood = false;
            } else {
                ctx.out << "Applied preset '" << presetName << "' to " << camera->getSerialNumber() << std::endl;
            }
        }

        const std::string target = serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber;
        if (allGood) {
            ctx.out << "Applied preset '" << presetName << "' to " << target << std::endl;
        } else {
            ctx.err << "Some presets failed to apply for " << target << "." << std::endl;
        }
    }
} // namespace commands::preset
