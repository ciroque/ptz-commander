#include "ListCommand.h"
#include "../ResolveCameras.h"

#include <iostream>
#include <iomanip>

namespace commands::camera {
    void ListCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = commands::resolveCameras(ctx, "*");
        if (cameras.empty()) {
            return;
        }

        int NameColWidth = 28;
        int AliasColWidth = 20;
        int SerialNumberColWidth = 20;
        int ConnectedColWidth = 10;


        // Table header
        ctx.out << "Listing devices..." << std::endl << std::endl;
        ctx.out << std::left
            << std::setw(NameColWidth) << "Name" << " | "
            << std::setw(AliasColWidth) << "Alias" << " | "
            << std::setw(SerialNumberColWidth) << "Serial Number" << " | "
            << std::setw(ConnectedColWidth) << "Connected" << std::endl;
        
        ctx.out << std::string(NameColWidth, '-') << "-+-"
            << std::string(AliasColWidth, '-') << "-+-"
            << std::string(SerialNumberColWidth, '-') << "-+-"
            << std::string(ConnectedColWidth, '-') << std::endl;

        // Table rows
        for (const auto& camera : cameras) {
            ctx.out << std::left
                << std::setw(NameColWidth) << camera->getName() << " | "
                << std::setw(AliasColWidth) << camera->getAlias() << " | "
                << std::setw(SerialNumberColWidth) << camera->getSerialNumber() << " | "
                << std::setw(ConnectedColWidth) << (camera->isConnected() ? "Yes" : "No") << std::endl;
        }
    }
}
