#ifndef COMMANDS_PRESET_STORECOMMAND_H
#define COMMANDS_PRESET_STORECOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::preset {
    class StoreCommand : public Command {
    public:
        StoreCommand() : Command("preset", "store", "preset store <id|*> <name>",
                                 "Store current position as a named preset") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_PRESET_STORECOMMAND_H