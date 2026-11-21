#ifndef COMMANDS_CAMERA_LOGINCOMMAND_H
#define COMMANDS_CAMERA_LOGINCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../setup/ConsolePassword.h"

#include <utility>

namespace commands::camera {
    class LoginCommand : public Command {
    public:
        // Asks with the hidden reader when no password is given and none is
        // stored. A password on the command line is the script form.
        explicit LoginCommand(commands::setup::OnvifPasswordPrompt prompt = commands::setup::readHiddenOnvifPassword)
            : Command("camera", "login", "camera login <id> [password]",
                      "Set an ONVIF password, connect, and make that camera active. "
                      "Asks when the password is omitted and none is stored"),
              prompt_(std::move(prompt)) {}
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        commands::setup::OnvifPasswordPrompt prompt_;
    };
} // namespace commands::camera

#endif
