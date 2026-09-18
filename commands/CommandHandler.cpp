#include "CommandHandler.h"

#include "camera/AliasCommand.h"
#include "camera/ListCommand.h"
#include "camera/MoveCommand.h"
#include "camera/PushCommand.h"
#include "camera/ShowCommand.h"

#include "console/ClearCommand.h"

#include "preset/ApplyCommand.h"
#include "preset/DiscardCommand.h"
#include "preset/ListCommand.h"
#include "preset/StoreCommand.h"

#include "setup/BrowseCommand.h"
#include "setup/LoadCommand.h"
#include "setup/SaveCommand.h"

#include "scene/AddCommand.h"
#include "scene/ApplyCommand.h"
#include "scene/DiscardCommand.h"
#include "scene/ListCommand.h"
#include "scene/RemoveCommand.h"
#include "scene/ShowCommand.h"
#include "scene/TourCommand.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace commands {

    CommandHandler::CommandHandler() {
        addCommand(std::make_unique<camera::ListCommand>());
        addCommand(std::make_unique<camera::AliasCommand>());
        addCommand(std::make_unique<camera::MoveCommand>());
        addCommand(std::make_unique<camera::PushCommand>());
        addCommand(std::make_unique<camera::ShowCommand>());

        addCommand(std::make_unique<preset::ListCommand>());
        addCommand(std::make_unique<preset::ApplyCommand>());
        addCommand(std::make_unique<preset::DiscardCommand>());
        addCommand(std::make_unique<preset::StoreCommand>());

        addCommand(std::make_unique<setup::BrowseCommand>());
        addCommand(std::make_unique<setup::LoadCommand>());
        addCommand(std::make_unique<setup::SaveCommand>());

        addCommand(std::make_unique<scene::ListCommand>());
        addCommand(std::make_unique<scene::ShowCommand>());
        addCommand(std::make_unique<scene::AddCommand>());
        addCommand(std::make_unique<scene::RemoveCommand>());
        addCommand(std::make_unique<scene::DiscardCommand>());
        addCommand(std::make_unique<scene::ApplyCommand>());
        addCommand(std::make_unique<scene::TourCommand>());

        addCommand(std::make_unique<console::ClearCommand>());
    }

    void CommandHandler::addCommand(std::unique_ptr<Command> cmd) {
        Command* raw = cmd.get();
        if (raw->family().empty()) {
            rootVerbs_[raw->verb()] = raw;
            rootOrder_.push_back(raw);
        }
        else {
            auto& verbs = verbs_[raw->family()];
            if (verbs.empty()) {
                familyOrder_.push_back(raw->family());
            }
            verbs[raw->verb()] = raw;
            familyCommands_[raw->family()].push_back(raw);
        }
        owned_.push_back(std::move(cmd));
    }

    bool CommandHandler::isFamily(const std::string& name) const {
        return verbs_.find(name) != verbs_.end();
    }

    std::vector<std::string> CommandHandler::tokenize(const std::string& input) {
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        std::string token;
        while (ss >> token) {
            tokens.push_back(std::move(token));
        }
        return tokens;
    }

    std::string CommandHandler::joinArgs(const std::vector<std::string>& tokens, size_t start) {
        if (start >= tokens.size()) {
            return {};
        }
        std::string args = tokens[start];
        for (size_t i = start + 1; i < tokens.size(); ++i) {
            args.push_back(' ');
            args += tokens[i];
        }
        return args;
    }

    void CommandHandler::printHelpUsage(data::Context& ctx) const {
        ctx.err << "Usage: help";
        if (!familyOrder_.empty()) {
            ctx.err << " [";
            for (size_t i = 0; i < familyOrder_.size(); ++i) {
                if (i > 0) {
                    ctx.err << "|";
                }
                ctx.err << familyOrder_[i];
            }
            ctx.err << "]";
        }
        ctx.err << std::endl;
    }

    void CommandHandler::printRootHelp(data::Context& ctx) const {
        size_t width = 4; // "help", "exit"
        for (const auto& family : familyOrder_) {
            width = std::max(width, family.size());
        }
        for (const auto* cmd : rootOrder_) {
            width = std::max(width, cmd->verb().size());
        }
        width = std::max(width, size_t{20});

        auto line = [&](const std::string& name, const std::string& summary) {
            ctx.out << "  " << name
                    << std::string(width - name.size(), ' ')
                    << "  " << summary << "\n";
        };

        ctx.out << "Available root commands:\n";
        line("help", "Show this help message");
        for (const auto& family : familyOrder_) {
            line(family, "Show " + family + " command help");
        }
        for (const auto* cmd : rootOrder_) {
            line(cmd->verb(), cmd->summary());
        }
        line("exit", "Exit the application");
    }

    void CommandHandler::printFamilyHelp(data::Context& ctx, const std::string& family) const {
        const auto& cmds = familyCommands_.at(family);
        size_t width = 0;
        for (const auto* cmd : cmds) {
            width = std::max(width, cmd->usage().size());
        }

        std::string title = family;
        title[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(title[0])));
        ctx.out << title << " commands:\n";
        for (const auto* cmd : cmds) {
            ctx.out << "  " << cmd->usage()
                    << std::string(width - cmd->usage().size() + 2, ' ')
                    << cmd->summary() << "\n";
        }
    }

    void CommandHandler::execute(data::Context& ctx, const std::string& input) {
        const auto tokens = tokenize(input);
        if (tokens.empty()) {
            ctx.err << "No command provided." << std::endl;
            return;
        }

        const std::string& head = tokens[0];

        if (head == "help") {
            if (tokens.size() == 1) {
                printRootHelp(ctx);
                return;
            }
            if (tokens.size() == 2 && isFamily(tokens[1])) {
                printFamilyHelp(ctx, tokens[1]);
                return;
            }
            if (tokens.size() == 2) {
                ctx.err << "Unknown command: help " << tokens[1] << std::endl;
                printHelpUsage(ctx);
                return;
            }
            printHelpUsage(ctx);
            return;
        }

        if (isFamily(head)) {
            if (tokens.size() == 1 || tokens[1] == "help") {
                if (tokens.size() > 2) {
                    ctx.err << "Usage: " << head << " help" << std::endl;
                    return;
                }
                printFamilyHelp(ctx, head);
                return;
            }

            auto familyIt = verbs_.find(head);
            auto verbIt = familyIt->second.find(tokens[1]);
            if (verbIt == familyIt->second.end()) {
                ctx.err << "Unknown command: " << head << " " << tokens[1] << std::endl;
                printFamilyHelp(ctx, head);
                return;
            }
            verbIt->second->execute(ctx, joinArgs(tokens, 2));
            return;
        }

        auto rootIt = rootVerbs_.find(head);
        if (rootIt != rootVerbs_.end()) {
            rootIt->second->execute(ctx, joinArgs(tokens, 1));
            return;
        }

        ctx.err << "Unknown command: " << head << std::endl;
    }

} // commands
