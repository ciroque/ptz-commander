#ifndef SNAPSHOT_HELPCOMMAND_H
#define SNAPSHOT_HELPCOMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::snapshot {

class HelpCommand : public Command {
public:
    HelpCommand() { name = "snapshot help"; }
    void execute(data::Context&, const std::string&) override {
        std::cout << "Snapshot commands:\n";
        std::cout << "  snapshot apply <name> [delay_ms]   Apply preset to all cameras\n";
        std::cout << "  snapshot list                      List all preset names across cameras\n";
        std::cout << "  snapshot help                      Show this help message\n";
    }
};

} // namespace commands::snapshot

#endif // SNAPSHOT_HELPCOMMAND_H
