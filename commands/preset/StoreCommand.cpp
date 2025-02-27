#include "StoreCommand.h"
#include <iostream>

namespace commands::preset {
    void StoreCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset add <serialNumber> <name>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: preset add <serialNumber> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  // TODO: Multi-word names later

        auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
        if (!camera) {
            std::cout << "Camera not found: " << serialNumber << std::endl;
            return;
        }

        cameras::Preset preset = camera->getCurrentPos();
        preset.name = presetName;
        camera->AddPreset(preset);

        std::cout << "Added preset '" << presetName << "' for " << serialNumber << std::endl;
    }
}