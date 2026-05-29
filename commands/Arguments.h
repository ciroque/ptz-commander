#ifndef COMMANDS_ARGUMENTS_H
#define COMMANDS_ARGUMENTS_H

#include <sstream>
#include <string>
#include <vector>

namespace commands {

    /**
     * Splits a string into whitespace-separated tokens.
     * This is the shared implementation previously duplicated
     * across nearly every command class.
     */
    inline std::vector<std::string> splitArgs(const std::string& args) {
        std::vector<std::string> tokens;
        std::stringstream ss(args);
        std::string token;

        while (ss >> token) {
            tokens.push_back(token);
        }

        return tokens;
    }

} // namespace commands

#endif // COMMANDS_ARGUMENTS_H
