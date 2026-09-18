#ifndef COMMANDS_PRESET_DISCARDCOMMAND_H
#define COMMANDS_PRESET_DISCARDCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::preset {
    class DiscardCommand : public Command {
    public:
        DiscardCommand() : Command("preset", "discard", "preset discard <id|*> <name>",
                                   "Remove a named preset") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_DISCARDCOMMAND_H