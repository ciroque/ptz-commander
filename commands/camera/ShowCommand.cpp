#include "ShowCommand.h"
#include "../ResolveCameras.h"
#include <iostream>
#include <iomanip>

namespace commands::camera {
    void ShowCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: camera show <id|*>" << std::endl;
            return;
        }

        // Split args to get serial number or wildcard
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            ctx.err << "Usage: camera show <id|*>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        // Table output for all cameras
        for (const auto& camera : cameras) {
            auto ptz = camera->getCurrentPtz();
            ctx.out << "Camera details for " << camera->getSerialNumber() << ":" << std::endl << std::endl;
            ctx.out << std::left
                << std::setw(20) << "Field" << " | "
                << std::setw(30) << "Value" << std::endl;

            ctx.out << std::string(20, '-') << "-+-"
                << std::string(30, '-') << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Name" << " | "
                << std::setw(30) << camera->getName() << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Alias" << " | "
                << std::setw(30) << camera->getAlias() << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Serial Number" << " | "
                << std::setw(30) << camera->getSerialNumber() << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Connected" << " | "
                << std::setw(30) << (camera->isConnected() ? "Yes" : "No") << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Pan" << " | "
                << std::setw(30) << ptz.pan << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Tilt" << " | "
                << std::setw(30) << ptz.tilt << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Zoom" << " | "
                << std::setw(30) << ptz.zoom << std::endl;

            ctx.out << std::endl;  // Space between camera tables
        }
    }
}