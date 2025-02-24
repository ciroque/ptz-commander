//
// Created by steve on 2/24/2025.
//

#ifndef COMMAND_H
#define COMMAND_H
#include <string>

#include "../data/Context.h"

namespace commands {

class Command {
protected:
    std::string name;
public:
    virtual ~Command() = default;
    virtual void execute(data::Context& ctx, const std::string& args) = 0;
    std::string getName() { return name; }
};

} // commands

#endif //COMMAND_H
