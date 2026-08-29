#ifndef COMMANDS_SETUP_HELP_COMMAND_H
#define COMMANDS_SETUP_HELP_COMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::setup {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "setup help"; }
        void execute(data::Context& ctx, const std::string&) override {
            ctx.out << "Setup commands:\n";
            ctx.out << "  setup browse                   List .ptzc files in the user data directory\n";
            ctx.out << "  setup load [file]              Replace in-memory presets and scenes from a .ptzc file (default: presets.ptzc)\n";
            ctx.out << "  setup save [file]              Save current presets and scenes to a .ptzc file (default: presets.ptzc)\n";
        }
    };
}

#endif // COMMANDS_SETUP_HELP_COMMAND_H
