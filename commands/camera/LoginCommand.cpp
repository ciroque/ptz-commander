#include "LoginCommand.h"
#include "../ResolveCameras.h"

#include <algorithm>
#include <iostream>
#include <optional>

namespace commands::camera {
    namespace {

        void reportLogin(data::Context& ctx, const cameras::Camera& camera,
                         const cameras::onvif::OnvifAddResult& result) {
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

        // Host and user for the hidden prompt. The stored record is preferred
        // so the question names the account setup load already has.
        cameras::onvif::OnvifEndpoint promptEndpoint(const data::Context& ctx, const std::string& serial) {
            const auto it
                = std::find_if(ctx.onvifEndpoints.begin(), ctx.onvifEndpoints.end(),
                               [&](const cameras::onvif::OnvifEndpoint& ep) { return ep.serial() == serial; });
            if (it != ctx.onvifEndpoints.end()) {
                return *it;
            }
            cameras::onvif::OnvifEndpoint ep;
            const std::string prefix = "ONVIF:";
            const auto body = serial.substr(prefix.size());
            const auto colon = body.rfind(':');
            if (colon == std::string::npos) {
                ep.host = body;
                return ep;
            }
            ep.host = body.substr(0, colon);
            try {
                ep.port = std::stoi(body.substr(colon + 1));
            } catch (const std::exception&) {
                ep.port = 80;
            }
            return ep;
        }

        std::optional<std::string> storedPassword(const data::Context& ctx, const std::string& serial) {
            const auto it = std::find_if(
                ctx.onvifEndpoints.begin(), ctx.onvifEndpoints.end(),
                [&](const cameras::onvif::OnvifEndpoint& ep) { return ep.serial() == serial && !ep.password.empty(); });
            if (it == ctx.onvifEndpoints.end()) {
                return std::nullopt;
            }
            return it->password;
        }

    } // namespace

    void LoginCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty() || tokens.size() > 2 || tokens[0] == "*") {
            ctx.err << "Usage: camera login <id> [password]" << std::endl;
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
        const auto serial = camera.getSerialNumber();
        std::string password;
        if (tokens.size() == 2) {
            password = tokens[1];
        } else {
            if (serial.rfind("ONVIF:", 0) != 0) {
                ctx.err << serial << " is not an ONVIF camera" << std::endl;
                return;
            }
            if (const auto stored = storedPassword(ctx, serial)) {
                password = *stored;
            } else {
                const auto entered = prompt_ ? prompt_(promptEndpoint(ctx, serial)) : std::nullopt;
                if (!entered || entered->empty()) {
                    ctx.err << "No password entered." << std::endl;
                    return;
                }
                password = *entered;
            }
        }

        reportLogin(ctx, camera, ctx.loginOnvif(serial, password));
    }
} // namespace commands::camera
