#ifndef COMMANDS_CAMERA_LOGINCOMMAND_H
#define COMMANDS_CAMERA_LOGINCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"

namespace commands::camera {
    class LoginCommand : public Command {
    public:
        LoginCommand()
            : Command("camera", "login", "camera login <id> <password>",
                      "Set an ONVIF password, connect, and make that camera active") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::camera

#endif
