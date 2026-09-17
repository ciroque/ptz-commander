#ifndef COMMANDS_SCENE_TOURCOMMAND_H
#define COMMANDS_SCENE_TOURCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class TourCommand : public Command {
    public:
        TourCommand() { name = "scene tour"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_TOURCOMMAND_H
