#include "DiscardCommand.h"
#include "../ResolveCameras.h"
#include <iostream>

namespace commands::preset {
    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: preset discard <id|*> <name>" << std::endl;
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: preset discard <id|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  // TODO: Multi-word names later

        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        bool allFound = true;
        bool changed = false;
        for (auto& camera : cameras) {
            if (!camera->GetPresetByName(presetName)) {
                ctx.err << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << std::endl;
                allFound = false;
                continue;
            }
            camera->RemovePreset(presetName);
            changed = true;
        }

        if (changed) {
            ctx.notifySetupChanged();
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