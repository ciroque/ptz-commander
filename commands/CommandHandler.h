#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "Command.h"

#include "camera/AliasCommand.h"
#include "camera/HelpCommand.h"
#include "camera/ListCommand.h"
#include "camera/MoveCommand.h"
#include "camera/PushCommand.h"
#include "camera/ShowCommand.h"

#include "console/ClearCommand.h"

#include "preset/ApplyCommand.h"
#include "preset/DiscardCommand.h"
#include "preset/HelpCommand.h"
#include "preset/ListCommand.h"
#include "preset/LoadCommand.h"
#include "preset/SaveCommand.h"
#include "preset/StoreCommand.h"

#include "HelpCommand.h"

namespace commands {
    class CommandHandler {
        std::map<std::string, std::unique_ptr<Command>> commands;

    public:
        explicit CommandHandler(data::Context& ctx) {

            addCommand(std::make_unique<camera::AliasCommand>());
            addCommand(std::make_unique<camera::ListCommand>());
            addCommand(std::make_unique<camera::MoveCommand>());
			addCommand(std::make_unique<camera::PushCommand>());

            addCommand(std::make_unique<camera::ShowCommand>());

			addCommand(std::make_unique<console::ClearCommand>());
            
            addCommand(std::make_unique<preset::ApplyCommand>());
            addCommand(std::make_unique<preset::DiscardCommand>());
            addCommand(std::make_unique<preset::ListCommand>());
            addCommand(std::make_unique<preset::LoadCommand>());
            addCommand(std::make_unique<preset::SaveCommand>());
            addCommand(std::make_unique<preset::StoreCommand>());
            
            addCommand(std::make_unique<HelpCommand>());
            addCommand(std::make_unique<camera::HelpCommand>());
            addCommand(std::make_unique<preset::HelpCommand>());
        }

        void addCommand(std::unique_ptr<Command> cmd) {
            commands[cmd->getName()] = std::move(cmd);
        }

        void execute(data::Context& ctx, const std::string& input) {
            if (input.empty()) {
                std::cout << "No command provided." << std::endl;
                return;
            }

            // Split input: "camera move RMOWTHF7211JGR 35.0 90.0 1.1"
            std::string objectVerb, args;
            size_t firstSpace = input.find(' ');
            if (firstSpace == std::string::npos) {
                objectVerb = input;  // e.g., "list" (for flat commands, if any)
            }
            else {
                objectVerb = input.substr(0, firstSpace);  // "camera"
                std::string rest = input.substr(firstSpace + 1);
                size_t secondSpace = rest.find(' ');
                if (secondSpace != std::string::npos) {
                    objectVerb += " " + rest.substr(0, secondSpace);  // "camera move"
                    args = rest.substr(secondSpace + 1);  // "RMOWTHF7211JGR 35.0 90.0 1.1"
                }
                else {
                    objectVerb += " " + rest;  // "camera list"
                }
            }

            auto it = commands.find(objectVerb);
            if (it != commands.end()) {
                it->second->execute(ctx, args);
            }
            else {
                std::cout << "Unknown command: " << objectVerb << std::endl;
            }
        }
    };
} // commands

#endif // COMMANDHANDLER_H
