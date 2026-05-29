#ifndef COMMANDS_CAMERA_PUSHCOMMAND_H
#define COMMANDS_CAMERA_PUSHCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::camera {
    class PushCommand : public Command {
    public:
        PushCommand() { name = "camera push"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CAMERA_PUSHCOMMAND_H