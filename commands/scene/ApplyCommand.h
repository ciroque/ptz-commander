#ifndef COMMANDS_SCENE_APPLYCOMMAND_H
#define COMMANDS_SCENE_APPLYCOMMAND_H

#include "../Command.h"

namespace commands::scene {
    class ApplyCommand : public Command {
    public:
        ApplyCommand() : Command("scene", "apply", "scene apply <name>", "Apply each binding (setPosition)") {}
        void execute(data::Context& ctx, const std::string& args) override;
        // One scene. Tour steps call this so a step does not stop the tour.
        void applyNamed(data::Context& ctx, const std::string& sceneName);
    };
} // namespace commands::scene

#endif // COMMANDS_SCENE_APPLYCOMMAND_H
