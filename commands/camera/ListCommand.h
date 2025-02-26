#ifndef LISTCOMMAND_H
#define LISTCOMMAND_H
#include "../Command.h"

#include "../data/Context.h"

namespace commands::camera {

class ListCommand : public Command {
public:
    ListCommand() { name = "camera list"; }
    void execute(data::Context& ctx, const std::string& args) override;
};

} // commands

#endif //LISTCOMMAND_H
