#include "PresetStore.h"
#include "../core/Logger.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
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

    LoadStatus PresetStore::load(CameraManager& mgr, SceneStore& scenes, std::string filename) {
        filename = resolvePresetPath(std::move(filename));

        std::ifstream file(filename);
        if (!file.is_open()) {
            return LoadStatus::NotFound;
        }

        try {
            nlohmann::json j;
            file >> j;
            file.close();

            struct StagedCamera {
                std::optional<std::string> alias;
                std::vector<Preset> presets;
            };

            auto cameras = mgr.getCameras();
            std::map<std::string, StagedCamera> staged;

            for (const auto& camera : cameras) {
                std::string sn = camera->getSerialNumber();
                if (sn == "scenes" || !j.contains(sn)) {
                    continue;
                }

                auto camJson = j[sn];
                StagedCamera entry;

                if (camJson.contains("alias")) {
                    std::string alias = camJson["alias"].get<std::string>();
                    if (!alias.empty()) {
                        entry.alias = std::move(alias);
                    }
                }

                if (camJson.contains("presets") && camJson["presets"].is_object()) {
                    auto presetsJson = camJson["presets"];
                    for (auto it = presetsJson.begin(); it != presetsJson.end(); ++it) {
                        auto presetJson = it.value();

                        Preset preset;
                        preset.name = it.key();
                        preset.ptz.pan = presetJson["pan"].get<float>();
                        preset.ptz.tilt = presetJson["tilt"].get<float>();
                        preset.ptz.zoom = presetJson["zoom"].get<int>();
                        entry.presets.push_back(std::move(preset));
                    }
                }

                staged.emplace(std::move(sn), std::move(entry));
            }

            std::map<std::string, Scene> stagedScenes;
            if (j.contains("scenes")) {
                auto scenesJson = j["scenes"];
                if (!scenesJson.is_object()) {
                    return LoadStatus::ParseError;
                }
                for (auto sceneIt = scenesJson.begin(); sceneIt != scenesJson.end(); ++sceneIt) {
                    if (!sceneIt.value().is_object()) {
                        return LoadStatus::ParseError;
                    }
                    Scene scene;
                    scene.name = sceneIt.key();
                    for (auto bindIt = sceneIt.value().begin(); bindIt != sceneIt.value().end(); ++bindIt) {
                        scene.bindings[bindIt.key()] = bindIt.value().get<std::string>();
                    }
                    stagedScenes.emplace(scene.name, std::move(scene));
                }
            }

            for (auto& camera : cameras) {
                camera->ClearPresets();

                auto it = staged.find(camera->getSerialNumber());
                if (it == staged.end()) {
                    continue;
                }

                if (it->second.alias) {
                    auto takenBy = mgr.assignAlias(camera, *it->second.alias);
                    if (takenBy) {
                        core::Logger::warn("Skipped alias '" + *it->second.alias
                            + "' for " + camera->getSerialNumber()
                            + (takenBy->empty() ? ": invalid alias" : (": already used by " + *takenBy)));
                    }
                }
                for (const auto& preset : it->second.presets) {
                    camera->AddPreset(preset.name, preset);
                }
            }

            scenes.replaceAll(std::move(stagedScenes));
            return LoadStatus::Ok;
        }
        catch (const std::exception&) {
            return LoadStatus::ParseError;
        }
    }

    bool PresetStore::save(const CameraManager& mgr, const SceneStore& scenes, std::string filename) const {
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

        if (!scenes.empty()) {
            nlohmann::json scenesObj;
            for (const auto& [name, scene] : scenes.all()) {
                nlohmann::json bindings;
                for (const auto& [serial, presetName] : scene.bindings) {
                    bindings[serial] = presetName;
                }
                scenesObj[name] = bindings;
            }
            j["scenes"] = scenesObj;
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
