#ifndef COMMANDS_CAMERA_ADDCOMMAND_H
#define COMMANDS_CAMERA_ADDCOMMAND_H

#include "../Command.h"

namespace commands::camera {
    class AddCommand : public Command {
    public:
        AddCommand()
            : Command("camera", "add", "camera add visca-ip <host> [port] [profile]",
                      "Add a VISCA-over-IP camera (default port 52381, profile tenveo-30x); "
                      "`camera add serial <COMn> [baud] [address] [profile]` adds one on a COM port "
                      "(default 9600 baud, address 1, profile lumens-vc-a51p); "
                      "`camera add scanned <n...|*> [profile]` adds rows of the last `camera scan visca-ip`") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif
