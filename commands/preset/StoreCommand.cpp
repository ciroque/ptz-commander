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

        cameras::Ptz ptz = camera->getCurrentPtz();
        
		cameras::Preset preset(presetName, ptz);

        camera->AddPreset(presetName, preset);

        std::cout << "Added preset '" << presetName << "' for " << serialNumber << std::endl;
    }
}