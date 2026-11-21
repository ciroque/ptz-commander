#include "StoreCommand.h"
#include "../ConnectionLabel.h"
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

        int stored = 0;
        for (auto& camera : cameras) {
            // An inactive camera reports pan 0, tilt 0, zoom 0. Storing that
            // would replace a real preset.
            if (commands::refuseInactiveOnvif(ctx, *camera)) {
                continue;
            }
            cameras::Preset preset;
            preset.name = presetName;
            preset.ptz = camera->getCurrentPtz();
            camera->addPreset(presetName, preset);
            ++stored;
        }
        if (stored == 0) {
            return;
        }

        ctx.notifySetupChanged();
        ctx.out << "Stored preset '" << presetName << "' for ";
        if (serialNumber == "*") {
            ctx.out << stored << " camera" << (stored == 1 ? "" : "s");
        } else {
            ctx.out << serialNumber;
        }
        ctx.out << std::endl;
    }
} // namespace commands::preset
