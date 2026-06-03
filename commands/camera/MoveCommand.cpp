#include "MoveCommand.h"
#include <iostream>

namespace commands::camera {
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
            ctx.err << "Usage: camera move <serialNumber> <pan> <tilt> <zoom>" << std::endl;
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 4) {
            ctx.err << "Usage: camera move <serialNumber> <pan> <tilt> <zoom>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::list<std::shared_ptr<cameras::Camera>> cameras;

        if (serialNumber == "*") {
            cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                ctx.err << "No cameras found to move." << std::endl;
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

        try {
            cameras::Ptz ptz = parsePtz(tokens, 1, 3);  // pan, tilt, zoom (0-100)

            bool allGood = true;
            for (auto& camera : cameras) {
                if (!camera->setPosition(ptz.pan, ptz.tilt, ptz.zoom)) {
                    ctx.err << "Failed to move camera: " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                }
            }

            if (allGood) {
                if (serialNumber == "*") {
                    ctx.out << "Moved " << cameras.size() << " camera" << (cameras.size() > 1 ? "s" : "")
                        << " to pan: " << ptz.pan << ", tilt: " << ptz.tilt << ", zoom: " << ptz.zoom << std::endl;
                }
                else {
                    ctx.out << "Moved " << serialNumber << " to pan: " << ptz.pan << ", tilt: " << ptz.tilt
                        << ", zoom: " << ptz.zoom << std::endl;
                }
            }
            else {
                ctx.err << "Some cameras failed to move." << std::endl;
            }
        }
        catch (const std::invalid_argument& e) {
            ctx.err << "Error: " << e.what() << std::endl;
        }
    }
}