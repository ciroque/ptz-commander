#include "ApplyCommand.h"
#include "../ResolveCameras.h"
#include <iostream>
#include <iterator>
#include <thread>
#include <chrono>

namespace commands::preset {
    namespace {
        constexpr int kDefaultDelayMs = 2000;

        void printUsage(data::Context& ctx) {
            ctx.err << "Usage: preset apply <id|*> <name|*> [delay_ms]" << std::endl;
        }
    }

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

        std::string serialNumber = tokens[0];
        std::string presetNameOrWildcard = tokens[1];
        int delayMs = kDefaultDelayMs;

        if (tokens.size() > 2) {
            try {
                delayMs = std::stoi(tokens[2]);
                if (delayMs < 0) {
                    ctx.err << "Invalid delay_ms, using default " << kDefaultDelayMs << "ms" << std::endl;
                    delayMs = kDefaultDelayMs;
                }
            }
            catch (const std::exception&) {
                ctx.err << "Invalid delay_ms, using default " << kDefaultDelayMs << "ms" << std::endl;
                delayMs = kDefaultDelayMs;
            }
        }

        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        bool allGood = true;
        for (auto& camera : cameras) {
            if (presetNameOrWildcard == "*") {
                // Cycle through all presets for this camera
                auto presets = camera->getPresets();
                if (presets.empty()) {
                    ctx.err << "No presets found for " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                    continue;
                }

                for (auto it = presets.begin(); it != presets.end(); ++it) {
                    const auto& preset = *it;
                    if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                        ctx.err << "Failed to apply preset '" << preset->name << "' to " << camera->getSerialNumber() << std::endl;
                        allGood = false;
                    }
                    else {
                        ctx.out << "Applied preset '" << preset->name << "' to " << camera->getSerialNumber() << std::endl;
                    }
                    if (std::next(it) != presets.end() && delayMs > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                    }
                }
            }
            else {
                // Apply single preset by name
                const cameras::Preset* preset = camera->GetPresetByName(presetNameOrWildcard);
                if (!preset) {
                    ctx.err << "Preset not found: " << presetNameOrWildcard << " for " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                    continue;
                }

                if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                    ctx.err << "Failed to apply preset '" << presetNameOrWildcard << "' to " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                }
                else {
                    ctx.out << "Applied preset '" << presetNameOrWildcard << "' to " << camera->getSerialNumber() << std::endl;
                }
            }
        }

        const std::string target =
            serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber;
        if (allGood) {
            if (presetNameOrWildcard == "*") {
                ctx.out << "Applied presets to " << target << std::endl;
            }
            else {
                ctx.out << "Applied preset '" << presetNameOrWildcard << "' to " << target << std::endl;
            }
        }
        else {
            ctx.err << "Some presets failed to apply for " << target << "." << std::endl;
        }
    }
}
