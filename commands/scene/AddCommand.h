#ifndef COMMANDS_SCENE_ADDCOMMAND_H
#define COMMANDS_SCENE_ADDCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class AddCommand : public Command {
    public:
        AddCommand() { name = "scene add"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_SCENE_ADDCOMMAND_H
