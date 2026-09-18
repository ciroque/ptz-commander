#ifndef LISTCOMMAND_H
#define LISTCOMMAND_H
#include "../Command.h"

#include "../../data/Context.h"

namespace commands::camera {

class ListCommand : public Command {
public:
    ListCommand() : Command("camera", "list", "camera list", "List all cameras") {}
    void execute(data::Context& ctx, const std::string& args) override;
};

} // commands

#endif //LISTCOMMAND_H
