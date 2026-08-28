#ifndef COMMANDS_PRESET_BROWSECOMMAND_H
#define COMMANDS_PRESET_BROWSECOMMAND_H

#include "../Command.h"

namespace commands::preset {
    class BrowseCommand : public Command {
    public:
        BrowseCommand() { name = "preset browse"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_BROWSECOMMAND_H
