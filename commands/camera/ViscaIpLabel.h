#ifndef COMMANDS_CAMERA_VISCAIPLABEL_H
#define COMMANDS_CAMERA_VISCAIPLABEL_H

#include "../../data/Context.h"
#include <algorithm>
#include <optional>
#include <string>

namespace commands::camera {

    inline std::string viscaIpSerial(const std::string& host, int port) {
        return "VISCA-IP:" + host + ":" + std::to_string(port);
    }

    // The alias (or, without one, the serial) of the VISCA-IP camera at
    // host:port, or nullopt if no such camera has been added.
    inline std::optional<std::string> viscaIpAddedAs(const data::Context& ctx,
                                                     const std::string& host, int port) {
        const bool added = std::any_of(ctx.viscaIpEndpoints.begin(), ctx.viscaIpEndpoints.end(),
            [&](const auto& ep) { return ep.host == host && ep.port == port; });
        if (!added) {
            return std::nullopt;
        }
        const std::string serial = viscaIpSerial(host, port);
        const auto camera = ctx.cameraMgr.findBySerial(serial);
        return (camera && !camera->getAlias().empty()) ? camera->getAlias() : serial;
    }

}

#endif // COMMANDS_CAMERA_VISCAIPLABEL_H
