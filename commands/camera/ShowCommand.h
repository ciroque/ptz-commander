#ifndef COMMANDS_CAMERA_SHOWCOMMAND_H
#define COMMANDS_CAMERA_SHOWCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"
#include "../../cameras/Preset.h"

namespace commands::camera {
    class ShowCommand : public Command {
    public:
        ShowCommand() { name = "camera show"; }
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CAMERA_SHOWCOMMAND_H