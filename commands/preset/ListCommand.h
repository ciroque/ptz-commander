#ifndef COMMANDS_PRESET_LISTCOMMAND_H
#define COMMANDS_PRESET_LISTCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"
#include <iomanip>
#include <iostream>

namespace commands::preset {
    class ListCommand : public Command {
    public:
        ListCommand() : Command("preset", "list", "preset list", "List all presets") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_LISTCOMMAND_H