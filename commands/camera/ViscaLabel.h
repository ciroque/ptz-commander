#ifndef COMMANDS_CAMERA_VISCALABEL_H
#define COMMANDS_CAMERA_VISCALABEL_H

#include "../../data/Context.h"
#include <algorithm>
#include <optional>
#include <string>

namespace commands::camera {

    inline std::string viscaIpSerial(const std::string& host, int port) {
        return "VISCA-IP:" + host + ":" + std::to_string(port);
    }

    // The camera's alias, or its serial if it has none.
    inline std::string cameraLabel(const data::Context& ctx, const std::string& serial) {
        const auto camera = ctx.cameraMgr.findBySerial(serial);
        return (camera && !camera->getAlias().empty()) ? camera->getAlias() : serial;
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
        return cameraLabel(ctx, viscaIpSerial(host, port));
    }

    // The same for the serial camera on a COM port, whatever its address.
    inline std::optional<std::string> viscaSerialAddedAs(const data::Context& ctx, const std::string& port) {
        const auto ep = std::find_if(ctx.viscaSerialEndpoints.begin(), ctx.viscaSerialEndpoints.end(),
            [&](const auto& candidate) { return candidate.port == port; });
        if (ep == ctx.viscaSerialEndpoints.end()) {
            return std::nullopt;
        }
        return cameraLabel(ctx, cameras::visca::serialCameraId(*ep));
    }

}

#endif // COMMANDS_CAMERA_VISCALABEL_H
