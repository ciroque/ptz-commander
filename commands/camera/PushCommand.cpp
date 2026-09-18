#include "PushCommand.h"
#include "../ResolveCameras.h"
#include <iostream>

namespace commands::camera {
    void PushCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: camera push <id|*> <targetZoom> [speed]" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        int targetZoom = 0;
        try {
            targetZoom = std::stoi(tokens[1]);
        }
        catch (const std::exception&) {
            ctx.err << "Invalid target zoom value." << std::endl;
            return;
        }

        int speed = 50;  // Default speed (0-100)
        if (tokens.size() > 2) {
            try {
                speed = std::stoi(tokens[2]);
            }
            catch (const std::exception&) {
                ctx.err << "Invalid speed value, using default 50." << std::endl;
            }
        }

        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        bool allGood = true;
        for (auto& camera : cameras) {
			auto currentPtz = camera->getCurrentPtz();
            if (currentPtz.zoom >= targetZoom) {
				ctx.out << "Camera " << camera->getSerialNumber() << " already at or past target zoom." << std::endl;
				continue;
			}

            if (!camera->setZoom(targetZoom, speed)) {
                ctx.err << "Failed to push zoom for " << camera->getSerialNumber() << std::endl;
                allGood = false;
            }
            else {
                ctx.out << "Pushed zoom to " << targetZoom << " for " << camera->getSerialNumber() << std::endl;
            }
        }

        if (!allGood) {
            ctx.err << "Some cameras failed to push zoom." << std::endl;
        }
    }
}