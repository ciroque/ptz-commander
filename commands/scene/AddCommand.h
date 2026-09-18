#ifndef COMMANDS_SCENE_ADDCOMMAND_H
#define COMMANDS_SCENE_ADDCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class AddCommand : public Command {
    public:
        AddCommand() : Command("scene", "add",
                               "scene add <name> <id> <preset> [<id> <preset> ...]",
                               "Add camera/preset bindings") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_ADDCOMMAND_H
