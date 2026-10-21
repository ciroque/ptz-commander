#ifndef COMMANDS_CAMERA_SCANCOMMAND_H
#define COMMANDS_CAMERA_SCANCOMMAND_H

#include "../Command.h"

namespace commands::camera {
    class ScanCommand : public Command {
    public:
        ScanCommand()
            : Command("camera", "scan", "camera scan visca-ip [subnet] [port]",
                      "Find VISCA-over-IP cameras on the local networks (or a subnet such as 192.168.1.0/24; default port 52381); "
                      "`camera scan serial [COMn...]` finds VISCA cameras on COM ports") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CAMERA_SCANCOMMAND_H
