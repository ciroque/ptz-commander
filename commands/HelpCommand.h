#ifndef COMMANDS_HELP_COMMAND_H
#define COMMANDS_HELP_COMMAND_H

#include "Command.h"
#include <iostream>

namespace commands {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "help"; }
        void execute(data::Context& ctx, const std::string&) override {
            ctx.out << "Available root commands:\n";
            ctx.out << "  help                  Show this help message\n";
            ctx.out << "  camera                Show camera command help\n";
            ctx.out << "  preset                Show preset command help\n";
            ctx.out << "  setup                 Show setup command help\n";
            ctx.out << "  scene                 Show scene command help\n";
            ctx.out << "  snapshot              Show snapshot command help\n";
            ctx.out << "  cls                   Clear the screen\n";
            ctx.out << "  exit                  Exit the application\n";
        }
    };
}

#endif // COMMANDS_HELP_COMMAND_H