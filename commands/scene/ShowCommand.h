#ifndef COMMANDS_SCENE_SHOWCOMMAND_H
#define COMMANDS_SCENE_SHOWCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class ShowCommand : public Command {
    public:
        ShowCommand() : Command("scene", "show", "scene show <name>", "Show bindings in a scene") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_SHOWCOMMAND_H
