#include "ApplyCommand.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace commands::snapshot {

    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: snapshot apply <name> [delay_ms]" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.empty()) {
            std::cout << "Usage: snapshot apply <name> [delay_ms]" << std::endl;
            return;
        }

        std::string presetName = tokens[0];
        int delayMs = 2000;  // Default 2000ms (2 seconds)

        // Parse optional delay (if present)
        if (tokens.size() > 1) {
            try {
                delayMs = std::stoi(tokens[1]);
                if (delayMs < 0) delayMs = 2000;  // Use default if negative
            }
            catch (const std::exception&) {
                std::cout << "Invalid delay_ms, using default 2000ms" << std::endl;
            }
        }

        // Get ALL cameras (snapshot applies to all cameras by definition)
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            std::cout << "No cameras found to apply preset." << std::endl;
            return;
        }

        bool allGood = true;
        int appliedCount = 0;
        for (auto& camera : cameras) {
            const cameras::Preset* preset = camera->GetPresetByName(presetName);
            if (!preset) {
                std::cout << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << " (skipping)" << std::endl;
                continue;
            }

            if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                std::cout << "Failed to apply preset '" << presetName << "' to " << camera->getSerialNumber() << std::endl;
                allGood = false;
            }
            else {
                std::cout << "Applied preset '" << presetName << "' to " << camera->getSerialNumber() << std::endl;
                appliedCount++;
            }
        }

        if (appliedCount > 0 && allGood) {
            std::cout << "Snapshot '" << presetName << "' applied to " << appliedCount << " camera(s)" << std::endl;
        }
        else if (appliedCount > 0) {
            std::cout << "Snapshot '" << presetName << "' partially applied to " << appliedCount << " camera(s) - some cameras failed" << std::endl;
        }
        else {
            std::cout << "Snapshot '" << presetName << "' not applied - preset not found on any camera" << std::endl;
        }
    }

} // namespace commands::snapshot
