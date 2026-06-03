#include "PresetStore.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace cameras {

    std::string PresetStore::normalizeFilename(std::string filename) {
        if (filename.find('.') == std::string::npos) {
            filename += ".ptzc";
        }
        return filename;
    }

    bool PresetStore::load(CameraManager& mgr, std::string filename) {
        filename = normalizeFilename(std::move(filename));

        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        try {
            nlohmann::json j;
            file >> j;
            file.close();

            auto cameras = mgr.getCameras();
            for (auto& camera : cameras) {
                std::string sn = camera->getSerialNumber();
                if (j.contains(sn)) {
                    auto camJson = j[sn];

                    // alias (optional)
                    if (camJson.contains("alias")) {
                        std::string alias = camJson["alias"].get<std::string>();
                        if (!alias.empty()) {
                            camera->setAlias(alias);
                        }
                    }

                    // presets
                    if (camJson.contains("presets") && camJson["presets"].is_object()) {
                        auto presetsJson = camJson["presets"];
                        for (auto it = presetsJson.begin(); it != presetsJson.end(); ++it) {
                            std::string presetName = it.key();
                            auto presetJson = it.value();

                            Preset preset;
                            preset.name = presetName;
                            preset.ptz.pan = presetJson["pan"].get<float>();
                            preset.ptz.tilt = presetJson["tilt"].get<float>();
                            preset.ptz.zoom = presetJson["zoom"].get<int>();
                            camera->AddPreset(presetName, preset);
                        }
                    }
                }
            }

            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

    bool PresetStore::save(const CameraManager& mgr, std::string filename) const {
        filename = normalizeFilename(std::move(filename));

        nlohmann::json j;
        auto cameras = mgr.getCameras();

        for (const auto& camera : cameras) {
            nlohmann::json camEntry;

            std::string alias = camera->getAlias();
            if (!alias.empty()) {
                camEntry["alias"] = alias;
            }

            nlohmann::json presetsObj;
            for (const auto& preset : camera->getPresets()) {
                nlohmann::json presetJson;
                presetJson["name"] = preset->name;
                presetJson["pan"] = preset->ptz.pan;
                presetJson["tilt"] = preset->ptz.tilt;
                presetJson["zoom"] = preset->ptz.zoom;
                presetsObj[preset->name] = presetJson;
            }
            camEntry["presets"] = presetsObj;

            j[camera->getSerialNumber()] = camEntry;
        }

        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(2);
        file.close();
        return true;
    }

} // namespace cameras
