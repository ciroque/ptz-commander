#ifndef COMMANDS_HELP_COMMAND_H
#define COMMANDS_HELP_COMMAND_H

#include "Command.h"
#include <iostream>

namespace commands {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "help"; }
        void execute(data::Context&, const std::string&) override {
            std::cout << "Available root commands:\n";
            std::cout << "  help                Show this help message\n";
            std::cout << "  camera help         Show camera command help\n";
            std::cout << "  preset help         Show preset command help\n";
            std::cout << "  camera <command>    Camera operations\n";
            std::cout << "  preset <command>    Preset operations\n";
            std::cout << "  exit                Exit the application\n";
        }
    };
}

#endif // COMMANDS_HELP_COMMAND_H