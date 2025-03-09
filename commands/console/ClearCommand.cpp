#include "ClearCommand.h"
#include <iostream>

namespace commands::console {
    void ClearCommand::execute(data::Context& ctx, const std::string& args) {
        system("cls");
        std::cout << "PTZ Commander REPL - 'cls' to clear screen, 'exit' to quit\n";
    }
}