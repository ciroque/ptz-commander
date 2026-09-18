#ifndef COMMANDS_SETUP_LOADCOMMAND_H
#define COMMANDS_SETUP_LOADCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"

namespace commands::setup {
    class LoadCommand : public Command {
    public:
        LoadCommand() : Command("setup", "load", "setup load [file]",
                                "Replace in-memory presets and scenes from a .ptzc file (default: presets.ptzc)") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SETUP_LOADCOMMAND_H
