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

        std::string filename = "presets.ptzc";
        auto tokens = splitArgs(args);
        if (!tokens.empty()) {
            filename = tokens[0];
            if (filename.find('.') == std::string::npos) {
                filename += ".ptzc";
            }
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
                camJson[preset->name] = presetJson;
            }
            j[camera->getSerialNumber()] = camJson;
        }

        std::ofstream file(filename);
        if (file.is_open()) {
            file << j.dump(2);
            file.close();
            std::cout << "Saved presets to " << filename << " for " << cameras.size() << " cameras" << std::endl;
        }
        else {
            std::cout << "Failed to write to " << filename << std::endl;
        }
    }
}