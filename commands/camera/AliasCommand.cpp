#include "AliasCommand.h"
#include <iostream>

namespace commands::camera {
    void AliasCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: camera alias <serialNumber> <alias>" << std::endl;
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: camera alias <serialNumber> <alias>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string alias = tokens[1]; 

        auto camera = ctx.cameraMgr.findById(serialNumber);
        if (!camera) {
            ctx.err << "Camera not found: " << serialNumber << std::endl;
            return;
        }

        if (alias == "*") {
            ctx.err << "Alias cannot be '*'" << std::endl;
            return;
        }

        auto takenBy = ctx.cameraMgr.assignAlias(camera, alias);
        if (takenBy) {
            if (takenBy->empty()) {
                ctx.err << "Invalid alias" << std::endl;
            }
            else {
                ctx.err << "Alias '" << alias << "' is already used by " << *takenBy << std::endl;
            }
            return;
        }

        ctx.out << "Set alias '" << alias << "' for camera " << serialNumber << std::endl;
    }
}