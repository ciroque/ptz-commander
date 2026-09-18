#include "StoreCommand.h"
#include "../ResolveCameras.h"
#include <iostream>

namespace commands::preset {
    void StoreCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: preset store <id|*> <name>" << std::endl;
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: preset store <id|*> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  

        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        for (auto& camera : cameras) {
            cameras::Preset preset;
            preset.name = presetName;
            preset.ptz = camera->getCurrentPtz();
            camera->AddPreset(presetName, preset);
        }

        ctx.notifySetupChanged();
        ctx.out << "Stored preset '" << presetName << "' for "
            << (serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber)
            << std::endl;
    }
}