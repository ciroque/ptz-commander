#ifndef COMMANDS_SCENE_DISCARDCOMMAND_H
#define COMMANDS_SCENE_DISCARDCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class DiscardCommand : public Command {
    public:
        DiscardCommand() { name = "scene discard"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_DISCARDCOMMAND_H
