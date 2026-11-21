#ifndef COMMANDS_SETUP_LOADCOMMAND_H
#define COMMANDS_SETUP_LOADCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "ConsolePassword.h"

namespace commands::setup {
    class LoadCommand : public Command {
    public:
        // Called once per ONVIF camera that has no usable password, and again
        // after an auth fault, up to three times. nullopt or blank skips.
        // The REPL uses the hidden console reader. Tests pass a stub.
        // setup load is registered only on the REPL. The WebSocket accepts
        // scene apply and nothing else, so this prompt cannot block a client.
        using PasswordPrompt = OnvifPasswordPrompt;

        explicit LoadCommand(PasswordPrompt prompt = readHiddenOnvifPassword)
            : Command("setup", "load", "setup load [file]",
                      "Replace in-memory presets and scenes from a .ptzc file (default: presets.ptzc). "
                      "Asks when an ONVIF password is missing"),
              prompt_(std::move(prompt)) {}

        void execute(data::Context& ctx, const std::string& args) override;

    private:
        PasswordPrompt prompt_;
    };
} // namespace commands::setup

#endif // COMMANDS_SETUP_LOADCOMMAND_H
