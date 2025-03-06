#include "ApplyCommand.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace commands::preset {
    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset apply <serialNumber|*> <name|*> [delay_ms]" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.empty()) {
            std::cout << "Usage: preset apply <serialNumber|*> <name|*> [delay_ms]" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetNameOrWildcard = tokens[1];
        int delayMs = 2000;  // Default 2000ms (2 seconds)

        // Parse optional delay (if present)
        if (tokens.size() > 2) {
            try {
                delayMs = std::stoi(tokens[2]);
                if (delayMs < 0) delayMs = 5000;  // Minimum 2s
            }
            catch (const std::exception&) {
                std::cout << "Invalid delay_ms, using default 2000ms" << std::endl;
            }
        }

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found to apply preset." << std::endl;
                return;
            }
        }
        else {
            auto camera = ctx.cameraMgr.findById(serialNumber);
            if (!camera) {
                std::cout << "Camera not found: " << serialNumber << std::endl;
                return;
            }
            cameras.push_back(camera);
        }

        bool allGood = true;
        for (auto& camera : cameras) {
            if (presetNameOrWildcard == "*") {
                // Cycle through all presets for this camera
                auto presets = camera->getPresets();
                if (presets.empty()) {
                    std::cout << "No presets found for " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                    continue;
                }

                for (const auto& preset : presets) {
                    if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                        std::cout << "Failed to apply preset '" << preset->name << "' to " << camera->getSerialNumber() << std::endl;
                        allGood = false;
                    }
                    else {
                        std::cout << "Applied preset '" << preset->name << "' to " << camera->getSerialNumber() << std::endl;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                }
            }
            else {
                // Apply single preset by name
                const cameras::Preset* preset = camera->GetPresetByName(presetNameOrWildcard);
                if (!preset) {
                    std::cout << "Preset not found: " << presetNameOrWildcard << " for " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                    continue;
                }

                if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                    std::cout << "Failed to apply preset '" << presetNameOrWildcard << "' to " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                }
                else {
                    std::cout << "Applied preset '" << presetNameOrWildcard << "' to " << camera->getSerialNumber() << std::endl;
                }
            }
        }

        if (allGood) {
            std::cout << "Applied preset" << (presetNameOrWildcard == "*" ? "s" : " '") << presetNameOrWildcard
                << "' to " << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
                << std::endl;
        }
        else {
            std::cout << "Some presets failed to apply for " << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber) << "." << std::endl;
        }
    }
}