#ifndef COMMANDS_CONNECTION_LABEL_H
#define COMMANDS_CONNECTION_LABEL_H

#include "../data/Context.h"

namespace commands {

    // "Inactive" needs a password, including a stored one the camera rejected.
    // "Offline" has a stored password and is not connected for another reason,
    // or is an OBSBOT that is in the setup but not plugged in.
    // A VISCA camera that is not connected stays "No".
    inline const char* connectionLabel(const cameras::Camera& camera) {
        switch (camera.connectionState()) {
        case cameras::ConnectionState::Connected: return "Yes";
        case cameras::ConnectionState::NeedsLogin: return "Inactive";
        case cameras::ConnectionState::Offline:
        case cameras::ConnectionState::Unplugged: return "Offline";
        case cameras::ConnectionState::NotConnected: return "No";
        }
        return "No";
    }

    // True when the command must not move this camera. Prints why.
    // An unplugged OBSBOT is not an ONVIF login.
    inline bool refuseNeedsLogin(data::Context& ctx, const cameras::Camera& camera) {
        const auto serial = camera.getSerialNumber();
        switch (camera.connectionState()) {
        case cameras::ConnectionState::NeedsLogin:
            ctx.err << serial << " is inactive. Use: camera login " << serial << std::endl;
            return true;
        case cameras::ConnectionState::Offline:
            ctx.err << serial << " is offline. Use: camera login " << serial << std::endl;
            return true;
        case cameras::ConnectionState::Unplugged: ctx.err << serial << " is unplugged." << std::endl; return true;
        case cameras::ConnectionState::Connected:
        case cameras::ConnectionState::NotConnected: return false;
        }
        return false;
    }

} // namespace commands

#endif
