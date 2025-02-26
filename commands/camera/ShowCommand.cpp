#include "ShowCommand.h"
#include <iostream>
#include <iomanip>

namespace commands::camera {
    void ShowCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: camera show <serialNumber>" << std::endl;
            return;
        }

        // Split args to get serial number
        auto tokens = splitArgs(args);
        if (tokens.empty()) {
            std::cout << "Usage: camera show <serialNumber>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
        if (!camera) {
            std::cout << "Camera not found: " << serialNumber << std::endl;
            return;
        }

        // Get camera state
        auto cameraState = camera->getCurrentState();

        // Table output
        std::cout << "Camera details for " << serialNumber << ":" << std::endl << std::endl;
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
    }
}