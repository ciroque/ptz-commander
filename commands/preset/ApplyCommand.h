#ifndef COMMANDS_PRESET_APPLYCOMMAND_H
#define COMMANDS_PRESET_APPLYCOMMAND_H

#include "../Command.h"
#include "../../cameras/Camera.h"
#include <sstream>
#include <vector>
#include <iostream>

namespace commands::preset {
    class ApplyCommand : public Command {
    public:
        ApplyCommand() { name = "preset apply"; }
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

#endif // COMMANDS_PRESET_APPLYCOMMAND_H