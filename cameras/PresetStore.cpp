#include "PresetStore.h"
#include "onvif/OnvifEndpoint.h"
#include "visca/ViscaCalibration.h"
#include "visca/ViscaCamera.h"
#include "../core/Logger.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>
#include <optional>
#include <filesystem>
#include <cstdlib>
#include <stdexcept>

namespace cameras {

    std::string PresetStore::normalizeFilename(std::string filename) {
        if (std::filesystem::path(filename).extension().empty()) {
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

    namespace {
        // Camera entries saved with the manual's lumens-vc-a51p mapping carry
        // "lumens_mapping": 2. Entries without it were saved with the legacy
        // mapping, and their presets are converted on load. A camera-entry key
        // keeps the file loadable by builds that predate it.
        constexpr const char* kLumensMappingKey = "lumens_mapping";
        constexpr int kLumensMapping = 2;

        // A serial camera's baud rate and profile, in its camera entry rather
        // than a top-level key: builds that predate it reject unknown
        // top-level keys but ignore unknown camera-entry keys.
        constexpr const char* kViscaSerialKey = "visca_serial";

        const visca::ViscaCamera* onLumensProfile(const Camera& camera) {
            const auto* viscaCamera = dynamic_cast<const visca::ViscaCamera*>(&camera);
            if (viscaCamera && viscaCamera->profile().name == visca::lumensVcA51p().name) {
                return viscaCamera;
            }
            return nullptr;
        }

        nlohmann::json serializeSetup(const CameraManager& mgr, const SceneStore& scenes) {
            nlohmann::json j = nlohmann::json::object();
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
                if (onLumensProfile(*camera)) {
                    camEntry[kLumensMappingKey] = kLumensMapping;
                }

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

            return j;
        }

        nlohmann::json axisToJson(const visca::ViscaAxis& axis) {
            return {
                {"min_deg", axis.minDeg},
                {"max_deg", axis.maxDeg},
                {"min_raw", axis.minRaw},
                {"home_raw", axis.homeRaw},
                {"max_raw", axis.maxRaw},
                {"through_home", axis.throughHome},
                {"signed", axis.signedRaw}
            };
        }

        nlohmann::json calibrationToJson(const visca::ViscaProfile& profile) {
            return {
                {"pan", axisToJson(profile.pan)},
                {"tilt", axisToJson(profile.tilt)},
                {"zoom", {{"min_raw", profile.zoomMinRaw}, {"max_raw", profile.zoomMaxRaw}}}
            };
        }

        // Throws on missing or out-of-range fields; callers turn that into ParseError.
        uint16_t rawField(const nlohmann::json& j, const char* key) {
            const int value = j.at(key).get<int>();
            if (value < 0 || value > 0xFFFF) {
                throw std::out_of_range(std::string("raw position out of range: ") + key);
            }
            return static_cast<uint16_t>(value);
        }

        visca::ViscaAxis axisFromJson(const nlohmann::json& j) {
            visca::ViscaAxis axis;
            axis.minDeg = j.at("min_deg").get<float>();
            axis.maxDeg = j.at("max_deg").get<float>();
            axis.minRaw = rawField(j, "min_raw");
            axis.homeRaw = rawField(j, "home_raw");
            axis.maxRaw = rawField(j, "max_raw");
            axis.throughHome = j.at("through_home").get<bool>();
            axis.signedRaw = j.at("signed").get<bool>();
            return axis;
        }

        visca::ViscaProfile calibrationFromJson(const nlohmann::json& j) {
            visca::ViscaProfile profile;
            profile.name = "calibrated";
            profile.pan = axisFromJson(j.at("pan"));
            profile.tilt = axisFromJson(j.at("tilt"));
            profile.zoomMinRaw = rawField(j.at("zoom"), "min_raw");
            profile.zoomMaxRaw = rawField(j.at("zoom"), "max_raw");
            std::string error;
            if (!visca::validCalibration(profile, error)) {
                throw std::invalid_argument("invalid calibration: " + error);
            }
            return profile;
        }

        bool parseViscaIp(const nlohmann::json& j, std::vector<visca::ViscaIpEndpoint>& out) {
            if (!j.contains("visca_ip")) {
                return true;
            }
            if (!j["visca_ip"].is_array()) {
                return false;
            }
            for (const auto& item : j["visca_ip"]) {
                if (!item.is_object() || !item.contains("host") || !item["host"].is_string()) {
                    return false;
                }
                visca::ViscaIpEndpoint ep;
                ep.host = item["host"].get<std::string>();
                ep.port = item.value("port", 52381);
                ep.profile = item.value("profile", std::string("tenveo-30x"));
                if (item.contains("calibration")) {
                    try {
                        ep.calibration = calibrationFromJson(item["calibration"]);
                    }
                    catch (const std::exception& e) {
                        throw std::runtime_error("VISCA-IP camera " + ep.host + ": " + e.what());
                    }
                }
                out.push_back(std::move(ep));
            }
            return true;
        }

        bool parseOnvif(const nlohmann::json& j, std::vector<onvif::OnvifEndpoint>& out) {
            if (!j.contains("onvif")) {
                return true;
            }
            if (!j["onvif"].is_array()) {
                return false;
            }
            for (const auto& item : j["onvif"]) {
                if (!item.is_object() || !item.contains("host") || !item["host"].is_string()
                    || !item.contains("user") || !item["user"].is_string()
                    || !item.contains("password") || !item["password"].is_string()) {
                    return false;
                }
                onvif::OnvifEndpoint ep;
                ep.host = item["host"].get<std::string>();
                ep.port = item.value("port", 80);
                ep.user = item["user"].get<std::string>();
                ep.password = item["password"].get<std::string>();
                ep.servicePath = item.value("path", std::string("/onvif/device_service"));
                out.push_back(std::move(ep));
            }
            return true;
        }
    }

    std::string PresetStore::toJsonString(const CameraManager& mgr, const SceneStore& scenes) const {
        return serializeSetup(mgr, scenes).dump();
    }

    LoadStatus PresetStore::load(CameraManager& mgr, SceneStore& scenes, std::string filename) {
        StagedSetup staged;
        const auto status = read(std::move(filename), staged);
        if (status == LoadStatus::Ok) {
            apply(mgr, scenes, staged);
        }
        return status;
    }

    LoadStatus PresetStore::read(std::string filename, StagedSetup& out, std::string* error) const {
        filename = resolvePresetPath(std::move(filename));

        std::ifstream file(filename);
        if (!file.is_open()) {
            return LoadStatus::NotFound;
        }

        const auto parseError = [error](const std::string& reason) {
            if (error) {
                *error = reason;
            }
            return LoadStatus::ParseError;
        };

        try {
            nlohmann::json j;
            file >> j;
            file.close();

            StagedSetup staged;

            // Every camera entry is parsed, not only those for cameras that
            // are present now, so a malformed entry fails before anything
            // (including adding VISCA-IP cameras) is changed.
            for (auto camIt = j.begin(); camIt != j.end(); ++camIt) {
                const std::string& sn = camIt.key();
                if (sn == "scenes" || sn == "visca_ip" || sn == "onvif") {
                    continue;
                }
                // Skipping it instead would let apply() clear that camera's presets.
                if (!camIt.value().is_object()) {
                    return parseError("camera entry '" + sn + "' is not an object");
                }

                auto camJson = camIt.value();
                StagedSetup::Camera entry;

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

                entry.legacyLumensMapping = camJson.value(kLumensMappingKey, 1) < kLumensMapping;

                if (auto serial = visca::parseSerialCameraId(sn)) {
                    if (camJson.contains(kViscaSerialKey)) {
                        const auto& block = camJson[kViscaSerialKey];
                        if (!block.is_object()) {
                            return parseError("\"visca_serial\" of '" + sn + "' is not an object");
                        }
                        serial->baud = block.value("baud", visca::kSerialDefaultBaud);
                        serial->profile = block.value("profile", std::string(visca::kSerialDefaultProfile));
                        if (!visca::validSerialBaud(serial->baud)) {
                            return parseError("camera entry '" + sn + "' has baud " + std::to_string(serial->baud) +
                                              "; use one of " + visca::validSerialBauds());
                        }
                        if (block.contains("calibration")) {
                            try {
                                serial->calibration = calibrationFromJson(block["calibration"]);
                            }
                            catch (const std::exception& e) {
                                return parseError("serial camera " + sn + ": " + e.what());
                            }
                        }
                    }
                    const auto samePort = std::find_if(staged.viscaSerial.begin(), staged.viscaSerial.end(),
                        [&serial](const visca::ViscaSerialEndpoint& ep) { return ep.port == serial->port; });
                    if (samePort != staged.viscaSerial.end()) {
                        return parseError("cameras '" + visca::serialCameraId(*samePort) + "' and '" + sn +
                                          "' are on the same port; only one camera per port is supported");
                    }
                    staged.viscaSerial.push_back(std::move(*serial));
                } else if (camJson.contains(kViscaSerialKey)) {
                    return parseError("camera entry '" + sn + "' has \"visca_serial\" but its key isn't a serial "
                                      "camera such as VISCA:COM3:1");
                }

                staged.cameras.emplace(sn, std::move(entry));
            }

            auto& stagedScenes = staged.scenes;
            if (j.contains("scenes")) {
                auto scenesJson = j["scenes"];
                if (!scenesJson.is_object()) {
                    return parseError("\"scenes\" is not an object");
                }
                for (auto sceneIt = scenesJson.begin(); sceneIt != scenesJson.end(); ++sceneIt) {
                    if (!sceneIt.value().is_object()) {
                        return parseError("scene '" + sceneIt.key() + "' is not an object");
                    }
                    Scene scene;
                    scene.name = sceneIt.key();
                    for (auto bindIt = sceneIt.value().begin(); bindIt != sceneIt.value().end(); ++bindIt) {
                        scene.bindings[bindIt.key()] = bindIt.value().get<std::string>();
                    }
                    stagedScenes.emplace(scene.name, std::move(scene));
                }
            }

            if (!parseViscaIp(j, staged.viscaIp)) {
                return parseError("\"visca_ip\" must be a list of entries that each have a host");
            }
            if (!parseOnvif(j, staged.onvif)) {
                return parseError("\"onvif\" must be a list of entries that each have a host, user, and password");
            }

            out = std::move(staged);
            return LoadStatus::Ok;
        }
        catch (const std::exception& e) {
            return parseError(e.what());
        }
    }

    void PresetStore::apply(CameraManager& mgr, SceneStore& scenes, const StagedSetup& staged) const {
        for (auto& camera : mgr.getCameras()) {
            camera->ClearPresets();

            auto it = staged.cameras.find(camera->getSerialNumber());
            if (it == staged.cameras.end()) {
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
            // Presets saved with the legacy Lumens mapping keep pointing at
            // the same spot under the current one.
            const auto* lumens = it->second.legacyLumensMapping ? onLumensProfile(*camera) : nullptr;
            for (const auto& preset : it->second.presets) {
                if (lumens) {
                    camera->AddPreset(preset.name, Preset(preset.name,
                        visca::remapPtz(preset.ptz, visca::lumensVcA51pLegacy(), lumens->profile())));
                } else {
                    camera->AddPreset(preset.name, preset);
                }
            }
            if (lumens && !it->second.presets.empty()) {
                core::Logger::info("Converted " + std::to_string(it->second.presets.size()) + " presets for " +
                                   camera->getSerialNumber() + " from the legacy Lumens mapping");
            }
        }

        scenes.replaceAll(staged.scenes);
    }

    bool PresetStore::save(const CameraManager& mgr, const SceneStore& scenes,
                           const std::vector<visca::ViscaIpEndpoint>& viscaIp,
                           const std::vector<visca::ViscaSerialEndpoint>& viscaSerial,
                           const std::vector<onvif::OnvifEndpoint>& onvif,
                           std::string filename) const {
        filename = resolvePresetPath(std::move(filename));

        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j = serializeSetup(mgr, scenes);
        if (!viscaIp.empty()) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& ep : viscaIp) {
                nlohmann::json entry = {
                    {"host", ep.host},
                    {"port", ep.port},
                    {"profile", ep.profile}
                };
                if (ep.calibration) {
                    entry["calibration"] = calibrationToJson(*ep.calibration);
                }
                arr.push_back(std::move(entry));
            }
            j["visca_ip"] = std::move(arr);
        }
        if (!onvif.empty()) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& ep : onvif) {
                arr.push_back({
                    {"host", ep.host},
                    {"port", ep.port},
                    {"user", ep.user},
                    {"password", ep.password},
                    {"path", ep.servicePath}
                });
            }
            j["onvif"] = std::move(arr);
        }
        for (const auto& ep : viscaSerial) {
            nlohmann::json block = {
                {"baud", ep.baud},
                {"profile", ep.profile}
            };
            if (ep.calibration) {
                block["calibration"] = calibrationToJson(*ep.calibration);
            }
            j[visca::serialCameraId(ep)][kViscaSerialKey] = std::move(block);
        }

        file << j.dump(2);
        file.close();
        return true;
    }

} // namespace cameras
