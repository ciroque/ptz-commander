#include "ListCommand.h"

namespace commands::preset {
    void ListCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            ctx.err << "No cameras found." << std::endl;
            return;
        }

        bool anyPresets = false;
        for (const auto& camera : cameras) {
            auto presets = camera->getPresets();
            if (presets.empty()) continue;

            if (!anyPresets) {
                ctx.out << "Listing presets..." << std::endl << std::endl;
                anyPresets = true;
            }

            ctx.out << "Camera: " << camera->getSerialNumber() << " (" << camera->getFriendlyName() << ")" << std::endl;
            ctx.out << std::left
                << std::setw(20) << "Preset Name" << " | "
                << std::setw(10) << "Pan" << " | "
                << std::setw(10) << "Tilt" << " | "
                << std::setw(10) << "Zoom" << std::endl;
            ctx.out << std::string(20, '-') << "-+-"
                << std::string(10, '-') << "-+-"
                << std::string(10, '-') << "-+-"
                << std::string(10, '-') << std::endl;

            for (const auto& preset : presets) {
                ctx.out << std::left
                    << std::setw(20) << preset->name << " | "
                    << std::setw(10) << preset->ptz.pan << " | "
                    << std::setw(10) << preset->ptz.tilt << " | "
                    << std::setw(10) << preset->ptz.zoom << std::endl;
            }
            ctx.out << std::endl;
        }

        if (!anyPresets) {
            ctx.err << "No presets found." << std::endl;
        }
    }
}