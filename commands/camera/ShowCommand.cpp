#include "ShowCommand.h"
#include <iostream>
#include <iomanip>

namespace commands::camera {
    void ShowCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: camera show <serialNumber> (or * for all)" << std::endl;
            return;
        }

        // Split args to get serial number or wildcard
        auto tokens = splitArgs(args);
        if (tokens.empty()) {
            std::cout << "Usage: camera show <serialNumber> (or * for all)" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::list<std::shared_ptr<cameras::ICamera>> cameras;

        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found." << std::endl;
                return;
            }
        }
        else {
            auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
            if (!camera) {
                std::cout << "Camera not found: " << serialNumber << std::endl;
                return;
            }
            cameras.push_back(camera);
        }

        // Table output for all cameras
        for (const auto& camera : cameras) {
            auto cameraState = camera->getCurrentState();
            std::cout << "Camera details for " << camera->getSerialNumber() << ":" << std::endl << std::endl;
            std::cout << std::left
                << std::setw(20) << "Field" << " | "
                << std::setw(30) << "Value" << std::endl;
            std::cout << std::string(20, '-') << "-+-"
                << std::string(30, '-') << std::endl;

            std::cout << std::left
                << std::setw(20) << "Name" << " | "
                << std::setw(30) << camera->getName() << std::endl;
            std::cout << std::left
                << std::setw(20) << "Serial Number" << " | "
                << std::setw(30) << camera->getSerialNumber() << std::endl;
            std::cout << std::left
                << std::setw(20) << "Connected" << " | "
                << std::setw(30) << (camera->isConnected() ? "Yes" : "No") << std::endl;
            std::cout << std::left
                << std::setw(20) << "Roll" << " | "
                << std::setw(30) << cameraState.roll << std::endl;
            std::cout << std::left
                << std::setw(20) << "Pan" << " | "
                << std::setw(30) << cameraState.pan << std::endl;
            std::cout << std::left
                << std::setw(20) << "Pitch" << " | "
                << std::setw(30) << cameraState.pitch << std::endl;
            std::cout << std::left
                << std::setw(20) << "Zoom" << " | "
                << std::setw(30) << cameraState.zoom << std::endl;
            std::cout << std::endl;  // Space between camera tables
        }
    }
}