#ifndef COMMANDS_SCENE_CONFIRMPROMPT_H
#define COMMANDS_SCENE_CONFIRMPROMPT_H

#include <functional>
#include <optional>
#include <string>

namespace commands::scene {

    // Reads one line from the console. nullopt when stdin is not a console
    // or the line cannot be read. A blank line is an empty string.
    std::optional<std::string> readConfirmLine();

    using ConfirmPrompt = std::function<std::optional<std::string>()>;

} // namespace commands::scene

#endif // COMMANDS_SCENE_CONFIRMPROMPT_H
