#ifndef COMMANDS_PRESET_TOURCOMMAND_H
#define COMMANDS_PRESET_TOURCOMMAND_H

#include "../Command.h"

namespace commands::preset {
    class TourCommand : public Command {
    public:
        TourCommand()
            : Command("preset", "tour", "preset tour <id|*> [seconds]",
                      "Apply each preset in name order; wait between (default 7s). tour stop ends it") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::preset

#endif // COMMANDS_PRESET_TOURCOMMAND_H
