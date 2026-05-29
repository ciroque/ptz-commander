#ifndef COMMANDS_CAMERA_ALIASCOMMAND_H
#define COMMANDS_CAMERA_ALIASCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::camera {
    class AliasCommand : public Command {
    public:
        AliasCommand() { name = "camera alias"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CAMERA_ALIASCOMMAND_H