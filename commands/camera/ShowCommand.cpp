#include "ShowCommand.h"
#include <iostream>
#include <iomanip>

namespace commands::camera {
    void ShowCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            ctx.err << "Usage: camera show <serialNumber> (or * for all)" << std::endl;
            return;
        }

        // Split args to get serial number or wildcard
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            ctx.err << "Usage: camera show <serialNumber> (or * for all)" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::list<std::shared_ptr<cameras::Camera>> cameras;

        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                ctx.err << "No cameras found." << std::endl;
                return;
            }
        }
        else {
            auto camera = ctx.cameraMgr.findById(serialNumber);
            if (!camera) {
                ctx.err << "Camera not found: " << serialNumber << std::endl;
                return;
            }
            cameras.push_back(camera);
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
                << std::setw(20) << "Pitch" << " | "
                << std::setw(30) << ptz.tilt << std::endl;

            ctx.out << std::left
                << std::setw(20) << "Zoom" << " | "
                << std::setw(30) << ptz.zoom << std::endl;

            ctx.out << std::endl;  // Space between camera tables
        }
    }
}