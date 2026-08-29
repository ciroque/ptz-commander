#ifndef COMMANDS_CAMERA_HELP_COMMAND_H
#define COMMANDS_CAMERA_HELP_COMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::camera {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "camera help"; }
        void execute(data::Context& ctx, const std::string&) override {
            ctx.out << "Camera commands:\n";
            ctx.out << "  camera list                          List all cameras\n";
            ctx.out << "  camera alias <id> <alias>            Set a unique friendly alias for a camera\n";
            ctx.out << "  camera move <id|*> <pan> <tilt> <zoom>   Move camera(s) to a PTZ position\n";
            ctx.out << "  camera push <id|*> <zoom> [speed]        Gradually zoom camera(s) to target\n";
            ctx.out << "  camera show <id|*>                   Show details for one or all cameras\n";
        }
    };
}

#endif // COMMANDS_CAMERA_HELP_COMMAND_H