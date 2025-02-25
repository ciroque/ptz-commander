#include "ShowCommand.h"

#include "../cameras/CameraState.h"

#include <iostream>

namespace commands {
    void ShowCommand::execute(data::Context &ctx, const std::string &args) {
        std::cout << "ShowCommand::Execute " << args << std::endl;

        auto camera = ctx.cameraMgr.getFirstCamera();
        if (camera) {
            auto cameraState = camera->getCurrentState();

            std::cout << "Current State:\n" 
                << "\tname(" << camera->getName() << ")\n"
                << "\tSN(" << camera->getSerialNumber() << ")\n"
                << "\troll(" << cameraState.roll << ")\n"
                << "\tpitch(" << cameraState.pitch << ")\n" 
                << "\tpan(" << cameraState.pan << ")\n" 
                << "\tzoom(" << cameraState.zoom << ")" 
                << std::endl;  
        }
    }
} // commands