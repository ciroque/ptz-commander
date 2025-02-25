//
// Created by steve on 2/24/2025.
//

#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H
#include <iostream>
#include <map>
#include <memory>

#include "Command.h"
#include "ListCommand.h"
#include "SelectCommand.h"
#include "ShowCommand.h"

namespace commands {

class CommandHandler {
    std::map<std::string, std::unique_ptr<Command>> commands;
public:
    explicit CommandHandler(data::Context& ctx) {
        addCommand(std::make_unique<ListCommand>());
        addCommand(std::make_unique<ShowCommand>());
        addCommand(std::make_unique<SelectCommand>());
    }

    void addCommand(std::unique_ptr<Command> cmd) {
        commands[cmd->getName()] = std::move(cmd);
    }

    void execute(data::Context& ctx, const std::string& input) {
        std::string cmdName = input;
        std::string args;
        size_t spacePos = input.find(' ');
        if (spacePos != std::string::npos) {
            cmdName = input.substr(0, spacePos);
            args = input.substr(spacePos + 1);
        }

        auto it = commands.find(cmdName);
        if (it != commands.end()) {
            it->second->execute(ctx, args);
        } else {
            std::cout << "Unknown command: " << cmdName << std::endl;
        }
    }
};

} // commands

#endif //COMMANDHANDLER_H
