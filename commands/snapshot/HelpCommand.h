#ifndef COMMANDS_SNAPSHOT_HELPCOMMAND_H
#define COMMANDS_SNAPSHOT_HELPCOMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::snapshot {

class HelpCommand : public Command {
public:
    HelpCommand() { name = "snapshot help"; }
    void execute(data::Context& ctx, const std::string&) override {
        ctx.out << "Snapshot commands:\n";
        ctx.out << "  snapshot apply <name>   Apply a named preset to all cameras\n";
        ctx.out << "  snapshot list           List preset names across cameras\n";
        ctx.out << "  snapshot help           Show this help message\n";
    }
};

} // namespace commands::snapshot

#endif // COMMANDS_SNAPSHOT_HELPCOMMAND_H
