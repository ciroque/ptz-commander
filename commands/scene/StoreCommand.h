#ifndef COMMANDS_SCENE_STORECOMMAND_H
#define COMMANDS_SCENE_STORECOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class StoreCommand : public Command {
    public:
        StoreCommand()
            : Command("scene", "store", "scene store <name> <id> <preset> [<id> <preset> ...]",
                      "Store camera/preset bindings") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::scene

#endif // COMMANDS_SCENE_STORECOMMAND_H
