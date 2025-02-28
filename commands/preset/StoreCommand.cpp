#include "StoreCommand.h"
#include <iostream>

namespace commands::preset {
    void StoreCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset store <serialNumber|*> <name>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: preset store <serialNumber|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found to store preset." << std::endl;
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

        for (auto& camera : cameras) {
            cameras::Preset preset;
            preset.name = presetName;
            preset.ptz = camera->getCurrentPtz();
            camera->AddPreset(presetName, preset);
        }

        std::cout << "Stored preset '" << presetName << "' for "
            << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
            << std::endl;
    }
}