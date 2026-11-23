#ifndef COMMANDS_SCENE_TOURCOMMAND_H
#define COMMANDS_SCENE_TOURCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class TourCommand : public Command {
    public:
        TourCommand()
            : Command("scene", "tour", "scene tour [seconds] [--background]",
                      "Apply every scene in list order; wait between (default 7s). Blocks unless --background. "
                      "tour stop ends a background tour") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::scene

#endif // COMMANDS_SCENE_TOURCOMMAND_H
