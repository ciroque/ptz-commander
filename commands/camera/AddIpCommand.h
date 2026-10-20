#ifndef COMMANDS_CAMERA_ADDIPCOMMAND_H
#define COMMANDS_CAMERA_ADDIPCOMMAND_H

#include "../Command.h"

namespace commands::camera {
    class AddIpCommand : public Command {
    public:
        AddIpCommand()
            : Command("camera", "add", "camera add visca-ip <host> [port] [profile]",
                      "Add a VISCA-over-IP camera (default port 52381, profile tenveo-30x); "
                      "`camera add scanned <n...|*> [profile]` adds rows of the last `camera scan visca-ip`") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif
