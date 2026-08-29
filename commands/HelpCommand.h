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
            ctx.out << "  camera help           Show camera command help\n";
            ctx.out << "  preset help           Show preset command help\n";
            ctx.out << "  setup help            Show setup command help\n";
            ctx.out << "  scene help            Show scene command help\n";
            ctx.out << "  snapshot help         Show snapshot command help\n";
            ctx.out << "  cls                   Clear the screen\n";
            ctx.out << "  camera <command>      alias, list, move, push, show\n";
            ctx.out << "  preset <command>      apply, discard, list, store\n";
            ctx.out << "  setup <command>       browse, load, save\n";
            ctx.out << "  scene <command>       add, apply, discard, list, remove, show\n";
            ctx.out << "  snapshot <command>    apply, list\n";
            ctx.out << "  exit                  Exit the application\n";
        }
    };
}

#endif // COMMANDS_HELP_COMMAND_H