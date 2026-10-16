#ifndef COMMANDS_CONSOLE_VERSIONCOMMAND_H
#define COMMANDS_CONSOLE_VERSIONCOMMAND_H

#include "../Command.h"

namespace commands::console {
    class VersionCommand : public Command {
    public:
        VersionCommand()
            : Command("", "version", "version [check]",
                      "Print this build's version. 'version check' compares it with the latest release") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CONSOLE_VERSIONCOMMAND_H
