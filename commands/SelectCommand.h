//
// Created by steve on 2/24/2025.
//

#ifndef SELECTCOMMAND_H
#define SELECTCOMMAND_H
#include "Command.h"

namespace commands {

class SelectCommand : public Command {
public:
    SelectCommand() { name = "select"; }
    void execute(data::Context& ctx, const std::string& args) override;
};

} // commands

#endif //SELECTCOMMAND_H
