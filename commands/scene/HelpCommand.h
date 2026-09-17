#ifndef COMMANDS_SCENE_HELP_COMMAND_H
#define COMMANDS_SCENE_HELP_COMMAND_H

#include "../Command.h"
#include <iostream>

namespace commands::scene {
    class HelpCommand : public Command {
    public:
        HelpCommand() { name = "scene help"; }
        void execute(data::Context& ctx, const std::string&) override {
            ctx.out << "Scene commands:\n";
            ctx.out << "  scene list                                      List scenes\n";
            ctx.out << "  scene show <name>                               Show bindings in a scene\n";
            ctx.out << "  scene add <name> <id> <preset> [<id> <preset> ...]  Add camera/preset bindings\n";
            ctx.out << "  scene remove <name> <id>                        Remove a camera from a scene\n";
            ctx.out << "  scene discard <name>                            Delete a scene\n";
            ctx.out << "  scene apply <name>                              Apply each binding (setPosition)\n";
            ctx.out << "  scene tour [seconds]                            Apply every scene in list order; wait between (default 7s)\n";
        }
    };
}

#endif // COMMANDS_SCENE_HELP_COMMAND_H
