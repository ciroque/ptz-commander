#ifndef COMMANDS_CAMERA_SHOWCOMMAND_H
#define COMMANDS_CAMERA_SHOWCOMMAND_H

#include "../Command.h"
#include "../../cameras/ICamera.h"
#include "../../cameras/CameraState.h"
#include <sstream>
#include <vector>

namespace commands::camera {
    class ShowCommand : public Command {
    public:
        ShowCommand() { name = "camera show"; }
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        // Split args into tokens (still header-only, small enough)
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

#endif // COMMANDS_CAMERA_SHOWCOMMAND_H