#include "PresetStore.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstdlib>

namespace cameras {

    std::string PresetStore::normalizeFilename(std::string filename) {
        if (filename.find('.') == std::string::npos) {
            filename += ".ptzc";
        }
        return filename;
    }

    std::string PresetStore::getDefaultPresetDir() {
        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData) {
            return (std::filesystem::path(localAppData) / "PTZCommander").string();
        }
        // Fallback (should rarely happen)
        return ".";
    }

    std::string PresetStore::homeDirectory() {
        return getDefaultPresetDir();
    }

    std::vector<std::string> PresetStore::listHomeFiles() {
        std::vector<std::string> names;
        const std::filesystem::path dir = getDefaultPresetDir();
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec)) {
            return names;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) {
                break;
            }
            std::error_code fileEc;
            if (!entry.is_regular_file(fileEc) || fileEc) {
                continue;
            }
            if (entry.path().extension() == ".ptzc") {
                names.push_back(entry.path().filename().string());
            }
        }

        std::sort(names.begin(), names.end());
        return names;
    }

    std::string PresetStore::resolvePresetPath(std::string filename) {
        filename = normalizeFilename(std::move(filename));

        std::filesystem::path p(filename);
        if (p.has_parent_path() || p.is_absolute()) {
            // User provided a path (relative with directory or absolute) — respect it as-is
            return filename;
        }

        // Just a bare filename — place it in the user app data directory
        std::filesystem::path dir = getDefaultPresetDir();
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);  // best effort
        return (dir / p).string();
    }

    bool PresetStore::load(CameraManager& mgr, std::string filename) {
        filename = resolvePresetPath(std::move(filename));

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
        filename = resolvePresetPath(std::move(filename));

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
