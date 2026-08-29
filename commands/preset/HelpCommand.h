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
            ctx.out << "  preset apply <id|*> <name>      Apply a named preset to camera(s)\n";
            ctx.out << "  preset discard <id|*> <name>    Remove a named preset\n";
            ctx.out << "  preset store <id|*> <name>      Store current position as a named preset\n";
            ctx.out << "  preset browse                   List preset files in the user data directory\n";
            ctx.out << "  preset load <file>              Replace in-memory presets from a .ptzc file\n";
            ctx.out << "  preset save <file>              Save current presets to a .ptzc file\n";
        }
    };
}

#endif // COMMANDS_PRESET_HELP_COMMAND_H