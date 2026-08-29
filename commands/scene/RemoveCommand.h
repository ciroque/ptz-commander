#ifndef COMMANDS_SCENE_REMOVECOMMAND_H
#define COMMANDS_SCENE_REMOVECOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class RemoveCommand : public Command {
    public:
        RemoveCommand() { name = "scene remove"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_REMOVECOMMAND_H
