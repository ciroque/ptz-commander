#ifndef COMMANDS_PRESET_SAVECOMMAND_H
#define COMMANDS_PRESET_SAVECOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::preset {
    class SaveCommand : public Command {
    public:
        SaveCommand() { name = "preset save"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_SAVECOMMAND_H