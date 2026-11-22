#ifndef COMMANDS_CONNECTION_LABEL_H
#define COMMANDS_CONNECTION_LABEL_H

#include "../data/Context.h"

namespace commands {

    // "Inactive" is a camera that reports NeedsLogin. Other cameras that are
    // not connected stay "No".
    inline const char* connectionLabel(const cameras::Camera& camera) {
        switch (camera.connectionState()) {
        case cameras::ConnectionState::Connected: return "Yes";
        case cameras::ConnectionState::NeedsLogin: return "Inactive";
        case cameras::ConnectionState::NotConnected: return "No";
        }
        return "No";
    }

    // True when the command must not move this camera. Prints why.
    inline bool refuseNeedsLogin(data::Context& ctx, const cameras::Camera& camera) {
        if (camera.connectionState() != cameras::ConnectionState::NeedsLogin) {
            return false;
        }
        ctx.err << camera.getSerialNumber() << " is inactive. Use: camera login " << camera.getSerialNumber()
                << std::endl;
        return true;
    }

} // namespace commands

#endif
