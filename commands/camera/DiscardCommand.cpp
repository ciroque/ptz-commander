#include "DiscardCommand.h"
#include "../Arguments.h"

#include <iostream>

namespace commands::camera {
    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() != 1) {
            ctx.err << "Usage: camera discard <id>" << std::endl;
            return;
        }
        if (tokens[0] == "*") {
            ctx.err << "camera discard does not accept '*'" << std::endl;
            return;
        }
        auto camera = ctx.cameraMgr.findById(tokens[0]);
        if (!camera) {
            ctx.err << "Camera not found: " << tokens[0] << std::endl;
            return;
        }
        if (!ctx.discardCamera) {
            ctx.err << "camera discard is not available." << std::endl;
            return;
        }

        const auto serial = camera->getSerialNumber();
        const auto status = ctx.discardCamera(serial);
        if (status == data::Context::DiscardCameraStatus::Obsbot) {
            ctx.err << serial << " is plugged in. Unplug it first." << std::endl;
            return;
        }
        if (status == data::Context::DiscardCameraStatus::NotFound) {
            ctx.err << "Camera not found: " << tokens[0] << std::endl;
            return;
        }

        const auto emptied = ctx.sceneStore.removeCamera(serial);
        ctx.notifySetupChanged();
        ctx.out << "Discarded " << serial << std::endl;
        for (const auto& name : emptied) {
            ctx.out << "Scene '" << name << "' is empty." << std::endl;
        }
    }
} // namespace commands::camera
