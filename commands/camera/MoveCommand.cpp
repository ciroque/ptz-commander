#ifndef COMMANDS_CAMERA_MOVECOMMAND_H
#define COMMANDS_CAMERA_MOVECOMMAND_H

#include "../Command.h"
#include "../../cameras/Camera.h"
#include <sstream>
#include <vector>
#include <iostream>

namespace commands::camera {
    class MoveCommand : public Command {
    public:
        MoveCommand() { name = "camera move"; }
        void execute(data::Context& ctx, const std::string& args) override;

    private:
        // Split args into tokens
        std::vector<std::string> splitArgs(const std::string& args) const {
            std::vector<std::string> tokens;
            std::stringstream ss(args);
            std::string token;
            while (ss >> token) {
                tokens.push_back(token);
            }
            return tokens;
        }

        // Parse floats from tokens (starting at index)
        std::vector<float> parseFloats(const std::vector<std::string>& tokens, size_t start, size_t count) const {
            if (tokens.size() < start + count) {
                throw std::invalid_argument("Not enough arguments for move command");
            }
            std::vector<float> floats;
            for (size_t i = start; i < start + count; ++i) {
                try {
                    floats.push_back(std::stof(tokens[i]));
                }
                catch (const std::exception&) {
                    throw std::invalid_argument("Invalid float argument: " + tokens[i]);
                }
            }
            return floats;
        }
    };

    void MoveCommand::execute(data::Context& ctx, const std::string& args) {
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
            auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
            if (!camera) {
                std::cout << "Camera not found: " << serialNumber << std::endl;
                return;
            }
            cameras.push_back(camera);
        }

        try {
            auto ptz = parseFloats(tokens, 1, 3);  // pan, tilt, zoom
            float pan = ptz[0];
            float tilt = ptz[1];
            float zoom = ptz[2];

            bool allGood = true;
            for (auto& camera : cameras) {
                if (!camera->setPosition(pan, tilt, zoom)) {
                    std::cout << "Failed to move camera: " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                }
            }

            if (allGood) {
                if (serialNumber == "*") {
                    std::cout << "Moved " << cameras.size() << " camera" << (cameras.size() > 1 ? "s" : "")
                        << " to pan: " << pan << ", tilt: " << tilt << ", zoom: " << zoom << std::endl;
                }
                else {
                    std::cout << "Moved " << serialNumber << " to pan: " << pan << ", tilt: " << tilt << ", zoom: " << zoom << std::endl;
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

#endif // COMMANDS_CAMERA_MOVECOMMAND_H