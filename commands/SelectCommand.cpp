//
// Created by steve on 2/24/2025.
//

#include "SelectCommand.h"

#include <iostream>

namespace commands {
    void SelectCommand::execute(data::Context &ctx, const std::string &args) {
        std::cout << "SelectCommand::execute " << args << std::endl;

        auto camera = ctx.cameraMgr.getFirstCamera();
        if (camera) {
            camera->setPosition(90, -45.0, 1.5);
        }
    }
} // commands


