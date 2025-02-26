#include "ListCommand.h"

#include <iostream>
#include <ostream>
#include <thread>


#include <iomanip>  // For table magic

namespace commands::camera {
    void ListCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            std::cout << "No cameras found." << std::endl;
            return;
        }

        // Table header
        std::cout << "Listing devices..." << std::endl << std::endl;
        std::cout << std::left
            << std::setw(20) << "Name" << " | "
            << std::setw(20) << "Serial Number" << " | "
            << std::setw(10) << "Connected" << std::endl;
        std::cout << std::string(20, '-') << "-+-"
            << std::string(20, '-') << "-+-"
            << std::string(10, '-') << std::endl;

        // Table rows
        for (const auto& camera : cameras) {
            std::cout << std::left
                << std::setw(20) << camera->getName() << " | "
                << std::setw(20) << camera->getSerialNumber() << " | "
                << std::setw(10) << (camera->isConnected() ? "Yes" : "No") << std::endl;
        }
    }
}
