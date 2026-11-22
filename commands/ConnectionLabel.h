#ifndef COMMANDS_CONNECTION_LABEL_H
#define COMMANDS_CONNECTION_LABEL_H

#include "../data/Context.h"

namespace commands {

    // "Inactive" needs a password. "Offline" has one and is not connected.
    // Other cameras that are not connected stay "No".
    inline const char* connectionLabel(const cameras::Camera& camera) {
        switch (camera.connectionState()) {
        case cameras::ConnectionState::Connected: return "Yes";
        case cameras::ConnectionState::NeedsLogin: return "Inactive";
        case cameras::ConnectionState::Offline: return "Offline";
        case cameras::ConnectionState::NotConnected: return "No";
        }
        return "No";
    }

    // True when the command must not move this camera. Prints why.
    inline bool refuseNeedsLogin(data::Context& ctx, const cameras::Camera& camera) {
        const char* state = nullptr;
        switch (camera.connectionState()) {
        case cameras::ConnectionState::NeedsLogin: state = "inactive"; break;
        case cameras::ConnectionState::Offline: state = "offline"; break;
        case cameras::ConnectionState::Connected:
        case cameras::ConnectionState::NotConnected: return false;
        }
        if (state == nullptr) {
            return false;
        }
        ctx.err << camera.getSerialNumber() << " is " << state << ". Use: camera login " << camera.getSerialNumber()
                << std::endl;
        return true;
    }

} // namespace commands

#endif
