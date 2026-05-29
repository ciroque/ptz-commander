#ifndef COMMANDS_CAMERA_MOVECOMMAND_H
#define COMMANDS_CAMERA_MOVECOMMAND_H

#include "../Command.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"
#include "../../cameras/Preset.h"  // For Ptz

namespace commands::camera {
    class MoveCommand : public Command {
    public:
        MoveCommand() { name = "camera move"; }
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        cameras::Ptz parsePtz(const std::vector<std::string>& tokens, size_t start, size_t count) const;
    };
}

#endif // COMMANDS_CAMERA_MOVECOMMAND_H