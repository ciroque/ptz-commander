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

        int NameColWidth = 28;
        int AliasColWidth = 20;
        int SerialNumberColWidth = 20;
        int ConnectedColWidth = 10;


        // Table header
        std::cout << "Listing devices..." << std::endl << std::endl;
        std::cout << std::left
            << std::setw(NameColWidth) << "Name" << " | "
            << std::setw(AliasColWidth) << "Alias" << " | "
            << std::setw(SerialNumberColWidth) << "Serial Number" << " | "
            << std::setw(ConnectedColWidth) << "Connected" << std::endl;
        
        std::cout << std::string(NameColWidth, '-') << "-+-"
            << std::string(AliasColWidth, '-') << "-+-"
            << std::string(SerialNumberColWidth, '-') << "-+-"
            << std::string(ConnectedColWidth, '-') << std::endl;

        // Table rows
        for (const auto& camera : cameras) {
            std::cout << std::left
                << std::setw(NameColWidth) << camera->getName() << " | "
                << std::setw(AliasColWidth) << camera->getAlias() << " | "
                << std::setw(SerialNumberColWidth) << camera->getSerialNumber() << " | "
                << std::setw(ConnectedColWidth) << (camera->isConnected() ? "Yes" : "No") << std::endl;
        }
    }
}
