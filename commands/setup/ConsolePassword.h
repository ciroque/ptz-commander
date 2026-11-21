#ifndef COMMANDS_SETUP_CONSOLEPASSWORD_H
#define COMMANDS_SETUP_CONSOLEPASSWORD_H

#include "../../cameras/onvif/OnvifEndpoint.h"

#include <optional>
#include <string>

namespace commands::setup {

    // Reads one line with console echo off. Returns nullopt when stdin is not
    // a console, on EOF, or when the line is blank (skip this camera).
    // Restores the previous console mode on every path.
    std::optional<std::string> readHiddenOnvifPassword(const cameras::onvif::OnvifEndpoint& endpoint);

} // namespace commands::setup

#endif
