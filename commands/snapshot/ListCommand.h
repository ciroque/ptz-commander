#ifndef SNAPSHOT_LISTCOMMAND_H
#define SNAPSHOT_LISTCOMMAND_H

#include "../Command.h"

namespace commands::snapshot {

class ListCommand : public Command {
public:
    ListCommand() {
        name = "snapshot list";
    }

    void execute(data::Context& ctx, const std::string& args) override;
};

} // namespace commands::snapshot

#endif // SNAPSHOT_LISTCOMMAND_H
