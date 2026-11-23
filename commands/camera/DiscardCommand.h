#ifndef COMMANDS_CAMERA_DISCARDCOMMAND_H
#define COMMANDS_CAMERA_DISCARDCOMMAND_H

#include "../Command.h"

namespace commands::camera {
    class DiscardCommand : public Command {
    public:
        DiscardCommand() : Command("camera", "discard", "camera discard <id>", "Discard a camera from the setup") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::camera

#endif // COMMANDS_CAMERA_DISCARDCOMMAND_H
