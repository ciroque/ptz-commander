#include "MoveCommand.h"

#include <iostream>

void commands::camera::MoveCommand::execute(data::Context& ctx, const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: camera move <serialNumber> <pan> <tilt> <zoom>" << std::endl;
        return;
    }

    // Split args into tokens
    auto tokens = splitArgs(args);
    if (tokens.size() < 4) {
        std::cout << "Usage: camera move <serialNumber> <pan> <tilt> <zoom>" << std::endl;
        return;
    }

    // Extract serial number and PTZ values
    std::string serialNumber = tokens[0];
    try {
        auto ptz = parseFloats(tokens, 1, 3);  // pan, tilt, zoom
        float pan = ptz[0];
        float tilt = ptz[1];
        float zoom = ptz[2];

        // Find camera by serial number
        auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
        if (!camera) {
            std::cout << "Camera not found: " << serialNumber << std::endl;
            return;
        }

        // Move the camera via ICamera
        if (!camera->setPosition(pan, tilt, zoom)) {  // Note: Swap if pitch/yaw order differs
            std::cout << "Failed to move camera: " << serialNumber << std::endl;
            return;
        }

        std::cout << "Moved " << serialNumber << " to pan: " << pan << ", tilt: " << tilt << ", zoom: " << zoom << std::endl;
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}