#include "SaveCommand.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

namespace commands::preset {
    void SaveCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            std::cout << "No cameras found to save presets." << std::endl;
            return;
        }

        nlohmann::json j;
        for (const auto& camera : cameras) {
            nlohmann::json camJson;
            for (const auto& preset : camera->getPresets()) {
                nlohmann::json presetJson;
                presetJson["name"] = preset->name;
                presetJson["pan"] = preset->ptz.pan;
                presetJson["tilt"] = preset->ptz.tilt;
                presetJson["zoom"] = preset->ptz.zoom;
                camJson[preset->name] = presetJson;  // Map key as name
            }
            j[camera->getSerialNumber()] = camJson;
        }

        std::ofstream file("presets.json");
        if (file.is_open()) {
            file << j.dump(2);
            file.close();
            std::cout << "Saved presets to presets.json for " << cameras.size() << " cameras" << std::endl;
        }
        else {
            std::cout << "Failed to write to presets.json" << std::endl;
        }
    }
}