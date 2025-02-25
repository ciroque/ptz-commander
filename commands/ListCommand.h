#ifndef LISTCOMMAND_H
#define LISTCOMMAND_H
#include "Command.h"

#include "../data/Context.h"

namespace commands {

class ListCommand : public Command {
public:
    ListCommand() { name = "list"; }
    void execute(data::Context& ctx, const std::string& args) override;
};

} // commands

#endif //LISTCOMMAND_H
