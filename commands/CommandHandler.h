#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "Command.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace commands {
    class CommandHandler {
    public:
        CommandHandler();
        void execute(data::Context& ctx, const std::string& input);

    private:
        void addCommand(std::unique_ptr<Command> cmd);
        bool isFamily(const std::string& name) const;
        void printRootHelp(data::Context& ctx) const;
        void printFamilyHelp(data::Context& ctx, const std::string& family) const;
        void printHelpUsage(data::Context& ctx) const;

        static std::vector<std::string> tokenize(const std::string& input);
        static std::string joinArgs(const std::vector<std::string>& tokens, size_t start);

        std::vector<std::unique_ptr<Command>> owned_;
        std::vector<std::string> familyOrder_;
        std::map<std::string, std::vector<Command*>> familyCommands_;
        std::map<std::string, std::map<std::string, Command*>> verbs_;
        std::vector<Command*> rootOrder_;
        std::map<std::string, Command*> rootVerbs_;
    };
} // commands

#endif // COMMANDHANDLER_H
