#include "SelectCommand.h"

#include <iostream>

void commands::SelectCommand::execute(data::Context &ctx, const std::string &args) 
{
    std::cout << "SelectCommand::execute " << args << std::endl;

    auto camera = ctx.cameraMgr.getFirstCamera();
    if (camera) {
        camera->setPosition(90, -45.0, 1.5);
    }
}
