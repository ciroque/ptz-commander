#ifndef COMMANDS_SETUP_CONSOLEPASSWORD_H
#define COMMANDS_SETUP_CONSOLEPASSWORD_H

#include "../../cameras/onvif/OnvifEndpoint.h"

#include <functional>
#include <optional>
#include <string>

namespace commands::setup {

    // nullopt or an empty string means the caller should change nothing.
    // The REPL passes readHiddenOnvifPassword. Tests pass a stub.
    using OnvifPasswordPrompt = std::function<std::optional<std::string>(const cameras::onvif::OnvifEndpoint&)>;

    // Reads one line with console echo off. Returns nullopt when stdin is not
    // a console, on EOF, or when the line is blank (skip this camera).
    // Restores the previous console mode on every path.
    std::optional<std::string> readHiddenOnvifPassword(const cameras::onvif::OnvifEndpoint& endpoint);

} // namespace commands::setup

#endif
