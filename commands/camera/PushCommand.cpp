#define NOMINMAX  // Prevent windows.h from defining min/max macros
#include "PushCommand.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>  // For Sleep
#include <algorithm>  // For std::min, std::max

namespace commands::camera {
    void PushCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: push <serialNumber|*> <targetZoom> [speed]" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        int targetZoom;
        try {
            targetZoom = std::stoi(tokens[1]);
        }
        catch (const std::exception&) {
            std::cout << "Invalid target zoom value." << std::endl;
            return;
        }

        int speed = 50;  // Default speed (0-100)
        if (tokens.size() > 2) {
            try {
                speed = std::stoi(tokens[2]);
            }
            catch (const std::exception&) {
                std::cout << "Invalid speed value, using default 50." << std::endl;
            }
        }

        std::list<std::shared_ptr<cameras::Camera>> cameras;
        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                std::cout << "No cameras found to push." << std::endl;
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

        bool allGood = true;
        for (auto& camera : cameras) {
            // Use setZoom method without limit checking
            if (!camera->setZoom(targetZoom, speed)) {
                std::cout << "Failed to push zoom for " << camera->getSerialNumber() << std::endl;
                allGood = false;
            }
            else {
                std::cout << "Pushed zoom to " << targetZoom << " for " << camera->getSerialNumber() << std::endl;
            }
        }

        if (!allGood) {
            std::cout << "Some cameras failed to push zoom." << std::endl;
        }
    }
}