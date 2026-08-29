#ifndef COMMANDS_SETUP_SAVECOMMAND_H
#define COMMANDS_SETUP_SAVECOMMAND_H

#include "../Command.h"
#include "../Arguments.h"

namespace commands::setup {
    class SaveCommand : public Command {
    public:
        SaveCommand() { name = "setup save"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SETUP_SAVECOMMAND_H
