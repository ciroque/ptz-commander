#ifndef COMMANDS_TOURCOMMAND_H
#define COMMANDS_TOURCOMMAND_H

#include "Command.h"

namespace commands {
    // Root command. A bare family name prints that family's help, so this
    // stays a root verb: `tour` and `tour stop`.
    class TourCommand : public Command {
    public:
        TourCommand() : Command("", "tour", "tour [stop]", "Show the running tour, or stop it") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands

#endif // COMMANDS_TOURCOMMAND_H
