#include "DiscardCommand.h"
#include <iostream>

namespace commands::preset {
    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset discard <serialNumber|*> <name>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: preset discard <serialNumber|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  // TODO: Multi-word names later

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found to discard preset." << std::endl;
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

        bool allFound = true;
        for (auto& camera : cameras) {
            if (!camera->GetPresetByName(presetName)) {
                std::cout << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << std::endl;
                allFound = false;
                continue;
            }
            camera->RemovePreset(presetName);
        }

        if (allFound) {
            std::cout << "Discarded preset '" << presetName << "' from "
                << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
                << std::endl;
        }
        else {
            std::cout << "Discarded preset '" << presetName << "' from some cameras." << std::endl;
        }
    }
}