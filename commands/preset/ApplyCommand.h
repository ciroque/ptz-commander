#ifndef COMMANDS_PRESET_APPLYCOMMAND_H
#define COMMANDS_PRESET_APPLYCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::preset {
    class ApplyCommand : public Command {
    public:
        ApplyCommand() { name = "preset apply"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_APPLYCOMMAND_H