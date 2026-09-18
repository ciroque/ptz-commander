#ifndef COMMANDS_SETUP_LISTCOMMAND_H
#define COMMANDS_SETUP_LISTCOMMAND_H

#include "../Command.h"

namespace commands::setup {
    class ListCommand : public Command {
    public:
        ListCommand() : Command("setup", "list", "setup list",
                                "List .ptzc files in the user data directory") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SETUP_LISTCOMMAND_H
