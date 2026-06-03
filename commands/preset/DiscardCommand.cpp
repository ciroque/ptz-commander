#include "DiscardCommand.h"
#include <iostream>

namespace commands::preset {
    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: preset discard <serialNumber|*> <name>" << std::endl;
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: preset discard <serialNumber|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  // TODO: Multi-word names later

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                ctx.err << "No cameras found to discard preset." << std::endl;
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

        bool allFound = true;
        for (auto& camera : cameras) {
            if (!camera->GetPresetByName(presetName)) {
                ctx.err << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << std::endl;
                allFound = false;
                continue;
            }
            camera->RemovePreset(presetName);
        }

        if (allFound) {
            ctx.out << "Discarded preset '" << presetName << "' from "
                << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
                << std::endl;
        }
        else {
            ctx.err << "Discarded preset '" << presetName << "' from some cameras." << std::endl;
        }
    }
}