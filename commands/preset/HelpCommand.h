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
            std::cout << "  preset list                     List all presets\n";
            std::cout << "  preset apply <id|*> <name>      Apply a named preset to camera(s)\n";
            std::cout << "  preset discard <id|*> <name>    Remove a named preset\n";
            std::cout << "  preset load <file>              Load a gig from .ptzc (per-camera: alias + presets)\n";
            std::cout << "  preset save <file>              Save current gig to .ptzc (per-camera: alias + presets object)\n";
            std::cout << "  preset store <id|*> <name>      Store current position as a named preset\n";
        }
    };
}

#endif // COMMANDS_PRESET_HELP_COMMAND_H