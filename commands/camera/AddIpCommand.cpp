#include "AddIpCommand.h"
#include "../Arguments.h"
#include "../../cameras/visca/ViscaIpEndpoint.h"
#include "../../cameras/visca/ViscaProfile.h"
#include <iostream>
#include <string>

namespace commands::camera {
    void AddIpCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2 || tokens[0] != "visca-ip") {
            ctx.err << "Usage: camera add visca-ip <host> [port] [profile]" << std::endl;
            return;
        }
        if (!ctx.addViscaIp) {
            ctx.err << "VISCA-IP is not available." << std::endl;
            return;
        }

        cameras::visca::ViscaIpEndpoint ep;
        ep.host = tokens[1];
        ep.port = 52381;
        ep.profile = "tenveo-30x";

        if (tokens.size() >= 3) {
            try {
                ep.port = std::stoi(tokens[2]);
            }
            catch (const std::exception&) {
                ctx.err << "Invalid port." << std::endl;
                return;
            }
            if (ep.port <= 0 || ep.port > 65535) {
                ctx.err << "Invalid port." << std::endl;
                return;
            }
        }
        if (tokens.size() >= 4) {
            ep.profile = tokens[3];
        }
        if (!cameras::visca::profileByName(ep.profile)) {
            ctx.err << "Unknown profile '" << ep.profile << "'. Known: "
                    << cameras::visca::knownProfileNames() << std::endl;
            return;
        }

        if (!ctx.addViscaIp(ep)) {
            ctx.err << "Could not add VISCA-IP camera " << ep.host << std::endl;
            return;
        }

        ctx.notifySetupChanged();
        ctx.out << "Added VISCA-IP:" << ep.host << ":" << ep.port
                << " profile=" << ep.profile << std::endl;
    }
}
