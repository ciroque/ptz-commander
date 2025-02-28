#include "ListCommand.h"

namespace commands::preset {
    void ListCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            std::cout << "No cameras found." << std::endl;
            return;
        }

        std::cout << "Listing presets..." << std::endl << std::endl;
        for (const auto& camera : cameras) {
            auto presets = camera->getPresets();
            if (presets.empty()) continue;  // Skip cameras with no presets

            std::cout << "Camera: " << camera->getSerialNumber() << " (" << camera->getFriendlyName() << ")" << std::endl;
            std::cout << std::left
                << std::setw(20) << "Preset Name" << " | "
                << std::setw(10) << "Pan" << " | "
                << std::setw(10) << "Tilt" << " | "
                << std::setw(10) << "Zoom" << std::endl;
            std::cout << std::string(20, '-') << "-+-"
                << std::string(10, '-') << "-+-"
                << std::string(10, '-') << "-+-"
                << std::string(10, '-') << std::endl;

            for (const auto& preset : presets) {
                std::cout << std::left
                    << std::setw(20) << preset->name << " | "
                    << std::setw(10) << preset->ptz.pan << " | "
                    << std::setw(10) << preset->ptz.tilt << " | "
                    << std::setw(10) << preset->ptz.zoom << std::endl;
            }
            std::cout << std::endl;  // Space between cameras
        }
    }
}