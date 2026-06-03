#include "StoreCommand.h"
#include <iostream>

namespace commands::preset {
    void StoreCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: preset store <serialNumber|*> <name>" << std::endl;
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: preset store <serialNumber|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                ctx.err << "No cameras found to store preset." << std::endl;
                return;
            }
        }
        else {
            auto camera = ctx.cameraMgr.findById(serialNumber);
            if (!camera) {
                ctx.err << "Camera not found: " << serialNumber << std::endl;
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

        ctx.out << "Stored preset '" << presetName << "' for "
            << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
            << std::endl;
    }
}