#ifndef COMMANDS_SETUP_SAVECOMMAND_H
#define COMMANDS_SETUP_SAVECOMMAND_H

#include "../Command.h"
#include "../Arguments.h"

namespace commands::setup {
    class SaveCommand : public Command {
    public:
        SaveCommand() : Command("setup", "save", "setup save [file]",
                                "Save current presets and scenes to a .ptzc file (default: presets.ptzc)") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SETUP_SAVECOMMAND_H
