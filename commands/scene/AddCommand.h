#ifndef COMMANDS_SCENE_ADDCOMMAND_H
#define COMMANDS_SCENE_ADDCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class AddCommand : public Command {
    public:
        // One release of the old name. Hidden from help.
        AddCommand()
            : Command("scene", "add", "scene add <name> <id> <preset> [<id> <preset> ...]",
                      "Store camera/preset bindings", false) {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::scene

#endif // COMMANDS_SCENE_ADDCOMMAND_H
