//
// Created by steve on 2/24/2025.
//

#include "ListCommand.h"

#include <iostream>
#include <ostream>

namespace commands {
    void ListCommand::execute(data::Context &ctx, const std::string &args) {
        std::cout << "ListCommand::Execute " << args << std::endl;
    }
} // commands
