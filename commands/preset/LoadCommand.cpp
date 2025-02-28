#include "LoadCommand.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>  // In $PROJ_DIR/include/nlohmann/

namespace commands::preset {
    void LoadCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            std::cout << "No cameras found to load presets." << std::endl;
            return;
        }

        std::ifstream file("presets.json");
        if (!file.is_open()) {
            std::cout << "Failed to open presets.json" << std::endl;
            return;
        }

        try {
            nlohmann::json j;
            file >> j;
            file.close();

            int loadedCount = 0;
            for (auto& camera : cameras) {
                std::string sn = camera->getSerialNumber();
                if (j.contains(sn)) {
                    auto camJson = j[sn];
                    for (auto it = camJson.begin(); it != camJson.end(); ++it) {
                        std::string presetName = it.key();
                        auto presetJson = it.value();
                        cameras::Preset preset;
						preset.name = presetName;
                        preset.ptz.pan = presetJson["pan"].get<float>();
                        preset.ptz.tilt = presetJson["tilt"].get<float>();
                        preset.ptz.zoom = presetJson["zoom"].get<int>();
                        camera->AddPreset(presetName, preset);
                        loadedCount++;
                    }
                }
            }

            std::cout << "Loaded " << loadedCount << " presets from presets.json for "
                << cameras.size() << " cameras" << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Error loading presets.json: " << e.what() << std::endl;
        }
    }
}
