#include "ApplyCommand.h"
#include <iostream>

namespace commands::preset {
    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset apply <serialNumber|*> <name>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: preset apply <serialNumber|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  // TODO: Multi-word names later

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found to apply preset." << std::endl;
                return;
            }
        }
        else {
            auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
            if (!camera) {
                std::cout << "Camera not found: " << serialNumber << std::endl;
                return;
            }
            cameras.push_back(camera);
        }

        bool allGood = true;
        for (auto& camera : cameras) {
            const cameras::Preset* preset = camera->GetPresetByName(presetName);
            if (!preset) {
                std::cout << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << std::endl;
                allGood = false;
                continue;
            }

            if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                std::cout << "Failed to apply preset '" << presetName << "' to " << camera->getSerialNumber() << std::endl;
                allGood = false;
            }
        }

        if (allGood) {
            std::cout << "Applied preset '" << presetName << "' to "
                << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
                << std::endl;
        }
        else {
            std::cout << "Some cameras failed to apply preset '" << presetName << "'." << std::endl;
        }
    }
}