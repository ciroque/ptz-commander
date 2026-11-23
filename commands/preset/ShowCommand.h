#ifndef COMMANDS_PRESET_SHOWCOMMAND_H
#define COMMANDS_PRESET_SHOWCOMMAND_H

#include "../Command.h"

namespace commands::preset {
    class ShowCommand : public Command {
    public:
        ShowCommand()
            : Command("preset", "show", "preset show <id|*> [name]",
                      "Show presets for one camera, or one named preset") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
} // namespace commands::preset

#endif // COMMANDS_PRESET_SHOWCOMMAND_H
