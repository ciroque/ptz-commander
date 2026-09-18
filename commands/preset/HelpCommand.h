#ifndef COMMANDS_PRESET_HELP_COMMAND_H
#define COMMANDS_PRESET_HELP_COMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::preset {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "preset help"; }
        void execute(data::Context& ctx, const std::string&) override {
            ctx.out << "Preset commands:\n";
            ctx.out << "  preset list                     List all presets\n";
            ctx.out << "  preset apply <id|*> <name|*> [delay_ms]  Apply a named preset, or cycle all (*)\n";
            ctx.out << "  preset discard <id|*> <name>    Remove a named preset\n";
            ctx.out << "  preset store <id|*> <name>      Store current position as a named preset\n";
        }
    };
}

#endif // COMMANDS_PRESET_HELP_COMMAND_H