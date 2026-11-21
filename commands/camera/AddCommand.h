#ifndef COMMANDS_CAMERA_ADDCOMMAND_H
#define COMMANDS_CAMERA_ADDCOMMAND_H

#include "../Command.h"
#include "../setup/ConsolePassword.h"

#include <utility>

namespace commands::camera {
    class AddCommand : public Command {
    public:
        // Omitting an ONVIF password asks with the hidden reader. A password
        // on the command line is the script form. The word after the user is
        // always the password, never a port.
        explicit AddCommand(commands::setup::OnvifPasswordPrompt prompt = commands::setup::readHiddenOnvifPassword)
            : Command("camera", "add", "camera add visca-ip <host> [port] [profile]",
                      "Add a VISCA-over-IP camera (default port 52381, profile tenveo-30x); "
                      "`camera add serial <COMn> [baud] [address] [profile]` adds one on a COM port "
                      "(default 9600 baud, address 1, profile lumens-vc-a51p); "
                      "`camera add onvif <host> <user> [password] [port]` adds an ONVIF camera "
                      "(default port 80) if it answers, and asks when the password is omitted; "
                      "`camera add scanned <n...|*> [profile]` adds VISCA rows of the last `camera scan`; "
                      "an ONVIF row takes `<user> [password]` instead, and `*` skips those rows until then"),
              prompt_(std::move(prompt)) {}
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        commands::setup::OnvifPasswordPrompt prompt_;
    };
} // namespace commands::camera

#endif
