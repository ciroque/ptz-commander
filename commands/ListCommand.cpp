#include "ListCommand.h"

#include <iostream>
#include <ostream>
#include <thread>



namespace commands {
    void ListCommand::execute(data::Context &ctx, const std::string &args) {
        std::cout << "Listing devices..." << std::endl << std::endl << args << std::endl;

        for (const auto& camera : ctx.cameraMgr.getCameras()) {
            std::cout << camera->getName() << " :: " << camera->getSerialNumber() << " :: " << camera->isConnected() << std::endl;
        }

    }
} // commands
