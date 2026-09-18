#ifndef COMMANDS_SETUP_BROWSECOMMAND_H
#define COMMANDS_SETUP_BROWSECOMMAND_H

#include "../Command.h"

namespace commands::setup {
    class BrowseCommand : public Command {
    public:
        BrowseCommand() : Command("setup", "browse", "setup browse",
                                  "List .ptzc files in the user data directory") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SETUP_BROWSECOMMAND_H
