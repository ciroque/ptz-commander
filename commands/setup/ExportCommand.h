#ifndef COMMANDS_SETUP_EXPORTCOMMAND_H
#define COMMANDS_SETUP_EXPORTCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"

namespace commands::setup {
    class ExportCommand : public Command {
    public:
        ExportCommand()
            : Command("setup", "export", "setup export <file>",
                      "Write a .ptzc file with the same setup and no ONVIF passwords") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::setup

#endif
