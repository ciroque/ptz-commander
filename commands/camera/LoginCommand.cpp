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

        // The stored password was rejected. Ask up to three times. A blank
        // answer leaves that password where it is.
        void promptReplacement(data::Context& ctx, const cameras::Camera& camera, const std::string& serial,
                               const commands::setup::OnvifPasswordPrompt& prompt) {
            const auto endpoint = promptEndpoint(ctx, serial);
            cameras::onvif::OnvifAddResult last;
            for (int attempt = 0; attempt < 3; ++attempt) {
                const auto entered = prompt ? prompt(endpoint) : std::nullopt;
                if (!entered || entered->empty()) {
                    ctx.err << "No password entered." << std::endl;
                    if (!camera.isConnected()) {
                        ctx.err << serial << " is still inactive" << std::endl;
                    }
                    return;
                }
                last = ctx.loginOnvif(serial, *entered);
                if (last.status == cameras::onvif::OnvifAddStatus::Added || !last.authRejected) {
                    reportLogin(ctx, camera, last);
                    return;
                }
                if (attempt + 1 < 3) {
                    ctx.err << "ONVIF password rejected for " << endpoint.user << "@" << endpoint.host << "; try again"
                            << std::endl;
                }
            }
            reportLogin(ctx, camera, last);
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
                const auto tried = ctx.loginOnvif(serial, *stored);
                if (tried.status == cameras::onvif::OnvifAddStatus::Added || !tried.authRejected) {
                    reportLogin(ctx, camera, tried);
                    return;
                }
                ctx.err << serial << ": the camera rejected the stored password" << std::endl;
                promptReplacement(ctx, camera, serial, prompt_);
                return;
            }
            const auto entered = prompt_ ? prompt_(promptEndpoint(ctx, serial)) : std::nullopt;
            if (!entered || entered->empty()) {
                ctx.err << "No password entered." << std::endl;
                return;
            }
            password = *entered;
        }

        reportLogin(ctx, camera, ctx.loginOnvif(serial, password));
    }
} // namespace commands::camera
