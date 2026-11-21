#include "LoginCommand.h"
#include "../ResolveCameras.h"

#include <iostream>

namespace commands::camera {
    void LoginCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() != 2 || tokens[0] == "*") {
            ctx.err << "Usage: camera login <id> <password>" << std::endl;
            return;
        }
        if (!ctx.loginOnvif) {
            ctx.err << "ONVIF is not available." << std::endl;
            return;
        }

        auto cameras = commands::resolveCameras(ctx, tokens[0]);
        if (cameras.size() != 1) {
            return;
        }

        const auto& camera = *cameras.front();
        const auto result = ctx.loginOnvif(camera.getSerialNumber(), tokens[1]);
        if (result.status == cameras::onvif::OnvifAddStatus::Added) {
            ctx.out << camera.getSerialNumber() << " is active" << std::endl;
            ctx.notifySetupChanged();
            return;
        }
        ctx.err << "Could not log in " << camera.getSerialNumber() << ": " << result.detail << std::endl;
        if (result.authRejected && !camera.isConnected()) {
            ctx.err << camera.getSerialNumber() << " is still inactive" << std::endl;
        }
    }
} // namespace commands::camera
