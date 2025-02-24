//
// Created by steve on 2/24/2025.
//

#ifndef SHOWCOMMAND_H
#define SHOWCOMMAND_H
#include "Command.h"

namespace commands {

class ShowCommand : public Command {
public:
    ShowCommand() { name = "show"; }
    void execute(data::Context& ctx, const std::string& args) override;
};

} // commands

#endif //SHOWCOMMAND_H
