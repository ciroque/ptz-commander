#include "AliasCommand.h"
#include <iostream>

namespace commands::camera {
    void AliasCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: camera alias <serialNumber> <alias>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: camera alias <serialNumber> <alias>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string alias = tokens[1]; 

        auto camera = ctx.cameraMgr.findById(serialNumber);
        if (!camera) {
            std::cout << "Camera not found: " << serialNumber << std::endl;
            return;
        }

        camera->setAlias(alias);
        std::cout << "Set alias '" << alias << "' for camera " << serialNumber << std::endl;
    }
}