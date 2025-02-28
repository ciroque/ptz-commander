#ifndef COMMANDS_PRESET_LOADCOMMAND_H
#define COMMANDS_PRESET_LOADCOMMAND_H

#include "../Command.h"
#include "../../cameras/Camera.h"
#include <sstream>
#include <vector>

namespace commands::preset {
    class LoadCommand : public Command {
    public:
        LoadCommand() { name = "preset load"; }
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        std::vector<std::string> splitArgs(const std::string& args) const {
            std::vector<std::string> tokens;
            std::stringstream ss(args);
            std::string token;
            while (ss >> token) {
                tokens.push_back(token);
            }
            return tokens;
        }
    };
}

#endif // COMMANDS_PRESET_LOADCOMMAND_H