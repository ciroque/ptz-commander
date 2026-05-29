#ifndef COMMANDS_SNAPSHOT_APPLYCOMMAND_H
#define COMMANDS_SNAPSHOT_APPLYCOMMAND_H

#include "../Command.h"
#include "../Arguments.h"

namespace commands::snapshot {

class ApplyCommand : public Command {
public:
    ApplyCommand() {
        name = "snapshot apply";
    }

    void execute(data::Context& ctx, const std::string& args) override;
};

} // namespace commands::snapshot

#endif // COMMANDS_SNAPSHOT_APPLYCOMMAND_H
