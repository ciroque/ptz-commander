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
            std::cout << "  camera list                 List all cameras\n";
            std::cout << "  camera move <id> <x> <y> <z> Move camera to position\n";
            std::cout << "  camera push <id>            Push camera state\n";
            std::cout << "  camera show <id>            Show camera details\n";
            std::cout << "  camera alias <id> <name>    Set camera alias\n";
        }
    };
}

#endif // COMMANDS_CAMERA_HELP_COMMAND_H