#include "MoveCommand.h"
#include <iostream>

namespace commands::camera {
    std::vector<std::string> MoveCommand::splitArgs(const std::string& args) const {
        std::vector<std::string> tokens;
        std::stringstream ss(args);
        std::string token;
        while (ss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    cameras::Ptz MoveCommand::parsePtz(const std::vector<std::string>& tokens, size_t start, size_t count) const {
        if (tokens.size() < start + count || count != 3) {
            throw std::invalid_argument("Not enough arguments for move command");
        }
        cameras::Ptz ptz;
        try {
            ptz.pan = std::stof(tokens[start]);
            ptz.tilt = std::stof(tokens[start + 1]);
            ptz.zoom = std::stoi(tokens[start + 2]);
            if (ptz.zoom < 0) ptz.zoom = 0;
            if (ptz.zoom > 100) ptz.zoom = 100;
        }
        catch (const std::exception& e) {
            throw std::invalid_argument(std::string("Invalid argument: ") + e.what());
        }
        return ptz;
    }

    void MoveCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: camera move <serialNumber> <pan> <tilt> <zoom>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 4) {
            std::cout << "Usage: camera move <serialNumber> <pan> <tilt> <zoom>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::list<std::shared_ptr<cameras::Camera>> cameras;

        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found to move." << std::endl;
                return;
            }
        }
        else {
            auto camera = ctx.cameraMgr.findById(serialNumber);
            if (!camera) {
                std::cout << "Camera not found: " << serialNumber << std::endl;
                return;
            }
            cameras.push_back(camera);
        }

        try {
            cameras::Ptz ptz = parsePtz(tokens, 1, 3);  // pan, tilt, zoom (0-100)

            bool allGood = true;
            for (auto& camera : cameras) {
                if (!camera->setPosition(ptz.pan, ptz.tilt, ptz.zoom)) {
                    std::cout << "Failed to move camera: " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                }
            }

            if (allGood) {
                if (serialNumber == "*") {
                    std::cout << "Moved " << cameras.size() << " camera" << (cameras.size() > 1 ? "s" : "")
                        << " to pan: " << ptz.pan << ", tilt: " << ptz.tilt << ", zoom: " << ptz.zoom << std::endl;
                }
                else {
                    std::cout << "Moved " << serialNumber << " to pan: " << ptz.pan << ", tilt: " << ptz.tilt
                        << ", zoom: " << ptz.zoom << std::endl;
                }
            }
            else {
                std::cout << "Some cameras failed to move." << std::endl;
            }
        }
        catch (const std::invalid_argument& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}