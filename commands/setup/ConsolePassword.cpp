#include "ConsolePassword.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <iostream>
#include <string>

namespace commands::setup {

    std::optional<std::string> readHiddenOnvifPassword(const cameras::onvif::OnvifEndpoint& endpoint) {
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        if (input == nullptr || input == INVALID_HANDLE_VALUE) {
            return std::nullopt;
        }
        DWORD mode = 0;
        if (!GetConsoleMode(input, &mode)) {
            return std::nullopt;
        }

        // Armed only after the mode actually changes, so a failed
        // SetConsoleMode does not write the old mode back over a race.
        struct ModeRestore {
            HANDLE input = nullptr;
            DWORD mode = 0;
            bool armed = false;
            ~ModeRestore() {
                if (armed) {
                    SetConsoleMode(input, mode);
                }
            }
        } restore{input, mode, false};

        if (!SetConsoleMode(input, mode & ~static_cast<DWORD>(ENABLE_ECHO_INPUT))) {
            return std::nullopt;
        }
        restore.armed = true;

        std::cout << "Password for " << endpoint.user << "@" << endpoint.host
                  << " (ONVIF, blank to skip): " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
            return std::nullopt;
        }
        std::cout << std::endl;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.find_first_not_of(" \t") == std::string::npos) {
            return std::nullopt;
        }
        return line;
    }

} // namespace commands::setup
