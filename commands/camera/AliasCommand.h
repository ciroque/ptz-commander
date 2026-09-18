#ifndef COMMANDS_CAMERA_ALIASCOMMAND_H
#define COMMANDS_CAMERA_ALIASCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::camera {
    class AliasCommand : public Command {
    public:
        AliasCommand() : Command("camera", "alias", "camera alias <id> <alias>",
                                 "Set a unique friendly alias for a camera") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CAMERA_ALIASCOMMAND_H