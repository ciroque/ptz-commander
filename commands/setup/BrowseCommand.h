#ifndef COMMANDS_SETUP_BROWSECOMMAND_H
#define COMMANDS_SETUP_BROWSECOMMAND_H

#include "../Command.h"

namespace commands::setup {
    class BrowseCommand : public Command {
    public:
        BrowseCommand() { name = "setup browse"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SETUP_BROWSECOMMAND_H
