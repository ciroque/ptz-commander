#ifndef COMMANDS_PRESET_TOURCOMMAND_H
#define COMMANDS_PRESET_TOURCOMMAND_H

#include "../Command.h"

namespace commands::preset {
    class TourCommand : public Command {
    public:
        TourCommand()
            : Command("preset", "tour", "preset tour <id|*> [seconds] [--background]",
                      "One camera after another, in camera list order; presets in name order; the wait "
                      "includes the gap between cameras (default 7s). Blocks unless --background. "
                      "tour stop ends a background tour") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::preset

#endif // COMMANDS_PRESET_TOURCOMMAND_H
