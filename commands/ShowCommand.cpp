//
// Created by steve on 2/24/2025.
//

#include "ShowCommand.h"

#include <iostream>

namespace commands {
    void ShowCommand::execute(data::Context &ctx, const std::string &args) {
        std::cout << "ShowCommand::Execute " << args << std::endl;
    }
} // commands