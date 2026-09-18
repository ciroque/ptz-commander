#ifndef COMMANDS_CONSOLE_CLEARCOMMAND_H
#define COMMANDS_CONSOLE_CLEARCOMMAND_H

#include "../Command.h"

namespace commands::console {
    class ClearCommand : public Command {
    public:
        ClearCommand() : Command("", "cls", "cls", "Clear the screen") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CONSOLE_CLEARCOMMAND_H