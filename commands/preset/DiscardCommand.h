#ifndef COMMANDS_PRESET_DISCARDCOMMAND_H
#define COMMANDS_PRESET_DISCARDCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::preset {
    class DiscardCommand : public Command {
    public:
        DiscardCommand() { name = "preset discard"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_DISCARDCOMMAND_H