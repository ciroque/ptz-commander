#include "ConfirmPrompt.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <iostream>
#include <string>

namespace commands::scene {

    std::optional<std::string> readConfirmLine() {
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        if (input == nullptr || input == INVALID_HANDLE_VALUE) {
            return std::nullopt;
        }
        DWORD mode = 0;
        if (!GetConsoleMode(input, &mode)) {
            return std::nullopt;
        }

        std::string line;
        if (!std::getline(std::cin, line)) {
            return std::nullopt;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return line;
    }

} // namespace commands::scene
