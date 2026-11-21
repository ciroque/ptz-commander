#ifndef COMMANDS_CONNECTION_LABEL_H
#define COMMANDS_CONNECTION_LABEL_H

#include "../data/Context.h"

namespace commands {

    // "Inactive" is an ONVIF camera kept in the setup without a connection.
    // Other cameras that are not connected stay "No".
    inline const char* connectionLabel(const cameras::Camera& camera) {
        if (camera.isConnected()) {
            return "Yes";
        }
        if (camera.getSerialNumber().rfind("ONVIF:", 0) == 0) {
            return "Inactive";
        }
        return "No";
    }

    // True when the command must not move this camera. Prints why.
    inline bool refuseInactiveOnvif(data::Context& ctx, const cameras::Camera& camera) {
        if (camera.isConnected() || camera.getSerialNumber().rfind("ONVIF:", 0) != 0) {
            return false;
        }
        ctx.err << camera.getSerialNumber() << " is inactive. Use: camera login " << camera.getSerialNumber()
                << " <password>" << std::endl;
        return true;
    }

} // namespace commands

#endif
