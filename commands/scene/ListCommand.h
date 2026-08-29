#ifndef COMMANDS_SCENE_LISTCOMMAND_H
#define COMMANDS_SCENE_LISTCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class ListCommand : public Command {
    public:
        ListCommand() { name = "scene list"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_LISTCOMMAND_H
