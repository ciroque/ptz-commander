#ifndef COMMANDS_SCENE_DISCARDCOMMAND_H
#define COMMANDS_SCENE_DISCARDCOMMAND_H

#include "../Command.h"
#include "ConfirmPrompt.h"

#include <utility>

namespace commands::scene {
    class DiscardCommand : public Command {
    public:
        // * asks at a console and deletes every scene only on y or Y.
        // The REPL uses the console reader. Tests pass a stub.
        explicit DiscardCommand(ConfirmPrompt prompt = readConfirmLine)
            : Command("scene", "discard", "scene discard <name|*>", "Discard a scene"), prompt_(std::move(prompt)) {}
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        ConfirmPrompt prompt_;
    };
} // namespace commands::scene

#endif // COMMANDS_SCENE_DISCARDCOMMAND_H
