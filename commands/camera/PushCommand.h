#ifndef COMMANDS_CAMERA_PUSHCOMMAND_H
#define COMMANDS_CAMERA_PUSHCOMMAND_H

#include "../Command.h"
#include "../../cameras/Camera.h"
#include <sstream>
#include <vector>

namespace commands::camera {
    class PushCommand : public Command {
    public:
        PushCommand() { name = "camera push"; }
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

#endif // COMMANDS_CAMERA_PUSHCOMMAND_H