#ifndef COMMANDS_CAMERA_MOVECOMMAND_H
#define COMMANDS_CAMERA_MOVECOMMAND_H

#include "../Command.h"
#include "../../cameras/Camera.h"
#include <sstream>
#include <vector>
#include <stdexcept>

namespace commands::camera {
    class MoveCommand : public Command {
    public:
        MoveCommand() { name = "camera move"; }
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        // Helper to split args into tokens
        std::vector<std::string> splitArgs(const std::string& args) const {
            std::vector<std::string> tokens;
            std::stringstream ss(args);
            std::string token;
            while (ss >> token) {
                tokens.push_back(token);
            }
            return tokens;
        }

        // Helper to parse floats from tokens (starting at index)
        std::vector<float> parseFloats(const std::vector<std::string>& tokens, size_t start, size_t count) const {
            if (tokens.size() < start + count) {
                throw std::invalid_argument("Not enough arguments for move command");
            }
            std::vector<float> floats;
            for (size_t i = start; i < start + count; ++i) {
                try {
                    floats.push_back(std::stof(tokens[i]));
                }
                catch (const std::exception&) {
                    throw std::invalid_argument("Invalid float argument: " + tokens[i]);
                }
            }
            return floats;
        }
    };
}

#endif // COMMANDS_CAMERA_MOVECOMMAND_H