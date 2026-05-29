#ifndef COMMANDS_CAMERA_HELP_COMMAND_H
#define COMMANDS_CAMERA_HELP_COMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::camera {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "camera help"; }
        void execute(data::Context&, const std::string&) override {
            std::cout << "Camera commands:\n";
            std::cout << "  camera list                          List all cameras\n";
            std::cout << "  camera alias <id> <alias>            Set a friendly alias for a camera\n";
            std::cout << "  camera move <id|*> <pan> <tilt> <zoom>   Move camera(s) to a PTZ position\n";
            std::cout << "  camera push <id|*> <zoom> [speed]        Gradually zoom camera(s) to target\n";
            std::cout << "  camera show <id|*>                   Show details for one or all cameras\n";
        }
    };
}

#endif // COMMANDS_CAMERA_HELP_COMMAND_H