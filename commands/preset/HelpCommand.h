#ifndef COMMANDS_PRESET_HELP_COMMAND_H
#define COMMANDS_PRESET_HELP_COMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::preset {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "preset help"; }
        void execute(data::Context&, const std::string&) override {
            std::cout << "Preset commands:\n";
            std::cout << "  preset list                 List all presets\n";
            std::cout << "  preset apply <id>           Apply a preset\n";
            std::cout << "  preset discard <id>         Discard a preset\n";
            std::cout << "  preset load <file>          Load presets from file\n";
            std::cout << "  preset save <file>          Save presets to file\n";
            std::cout << "  preset store <id>           Store current state as preset\n";
        }
    };
}

#endif // COMMANDS_PRESET_HELP_COMMAND_H