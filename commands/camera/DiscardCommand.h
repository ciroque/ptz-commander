#ifndef COMMANDS_CAMERA_DISCARDCOMMAND_H
#define COMMANDS_CAMERA_DISCARDCOMMAND_H

#include "../Command.h"
#include "../scene/ConfirmPrompt.h"

#include <utility>

namespace commands::camera {
    class DiscardCommand : public Command {
    public:
        // A camera with presets or scene bindings asks at a console and is
        // discarded only on y or Y. The REPL uses the console reader. Tests
        // pass a stub.
        explicit DiscardCommand(commands::scene::ConfirmPrompt prompt = commands::scene::readConfirmLine)
            : Command("camera", "discard", "camera discard <id>", "Discard a camera from the setup"),
              prompt_(std::move(prompt)) {}
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        commands::scene::ConfirmPrompt prompt_;
    };
} // namespace commands::camera

#endif // COMMANDS_CAMERA_DISCARDCOMMAND_H
