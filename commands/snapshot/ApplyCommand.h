#ifndef COMMANDS_SNAPSHOT_APPLYCOMMAND_H
#define COMMANDS_SNAPSHOT_APPLYCOMMAND_H

#include "../Command.h"
#include <sstream>
#include <vector>

namespace commands::snapshot {

class ApplyCommand : public Command {
public:
    ApplyCommand() {
        name = "snapshot apply";
    }

    void execute(data::Context& ctx, const std::string& args) override;

private:
    std::vector<std::string> splitArgs(const std::string& args) const {
        std::vector<std::string> tokens;
        std::istringstream stream(args);
        std::string token;
        while (stream >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

} // namespace commands::snapshot

#endif // COMMANDS_SNAPSHOT_APPLYCOMMAND_H
