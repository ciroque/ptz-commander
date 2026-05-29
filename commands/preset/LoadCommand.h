#ifndef COMMANDS_PRESET_LOADCOMMAND_H
#define COMMANDS_PRESET_LOADCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::preset {
    class LoadCommand : public Command {
    public:
        LoadCommand() { name = "preset load"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_LOADCOMMAND_H