#ifndef CAMERAS_PRESETSTORE_H
#define CAMERAS_PRESETSTORE_H

#include "CameraManager.h"
#include "SceneStore.h"
#include "visca/ViscaIpEndpoint.h"
#include "visca/ViscaSerialEndpoint.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cameras {

    /**
     * Handles persistence of presets and aliases to .ptzc files.
     * Each file contains per-serial camera data:
     * {
     *   "SERIAL": {
     *     "alias": "FriendlyName",
     *     "presets": {
     *       "name": { "name": "name", "pan": x, "tilt": y, "zoom": z },
     *       ...
     *     }
     *   },
     *   "VISCA:COM3:1": {
     *     "presets": { ... },
     *     "visca_serial": { "baud": 9600, "profile": "lumens-vc-a51p", "calibration": { ... } }
     *   },
     *   "scenes": {
     *     "sermon": { "SERIAL": "presetName", ... }
     *   }
     * }
     *
     * This is the central abstraction for preset file persistence.
     */
    enum class LoadStatus {
        Ok,
        NotFound,
        ParseError
    };

    /**
     * A .ptzc file parsed in full but not yet applied. Holds every camera
     * entry in the file, keyed by serial, whether or not that camera is
     * currently present.
     */
    struct StagedSetup {
        struct Camera {
            std::optional<std::string> alias;
            std::vector<Preset> presets;
            // No "lumens_mapping" marker: if this camera is on the
            // lumens-vc-a51p profile, its presets use the legacy mapping.
            bool legacyLumensMapping = true;
        };
        std::map<std::string, Camera> cameras;
        std::map<std::string, Scene> scenes;
        std::vector<visca::ViscaIpEndpoint> viscaIp;
        // One per VISCA:COMn:a camera entry; the port and address come from
        // the key.
        std::vector<visca::ViscaSerialEndpoint> viscaSerial;
    };

    class PresetStore {
    public:
        /**
         * Loads presets and aliases from the given file into the cameras
         * currently present in the manager (matched by serial number).
         * If filename has no path component, it is resolved relative to the
         * user-writable app data directory (%LOCALAPPDATA%\PTZCommander).
         * If filename has no '.', ".ptzc" is appended.
         * Replaces in-memory presets on currently connected cameras (matched by
         * serial). Cameras absent from the file are cleared. Aliases in the file
         * overwrite; missing alias keys are left unchanged.
         * File contents are fully parsed before any camera or scene is mutated;
         * a parse failure leaves in-memory state unchanged.
         * Replaces in-memory scenes from the reserved top-level "scenes" key.
         * A missing "scenes" key clears the scene store.
         */
        LoadStatus load(CameraManager& mgr, SceneStore& scenes,
                        std::string filename = "presets.ptzc");

        /**
         * Parses the whole file (camera entries, scenes, visca_ip) without
         * touching any state. load() is read() followed by apply(); callers
         * that must change cameras in between (adding VISCA-IP cameras) use
         * the two halves directly.
         * On ParseError, error (if given) says what was wrong.
         */
        LoadStatus read(std::string filename, StagedSetup& out,
                        std::string* error = nullptr) const;

        /**
         * Applies a staged setup to the cameras currently present, with the
         * same rules as load().
         */
        void apply(CameraManager& mgr, SceneStore& scenes, const StagedSetup& staged) const;

        /**
         * In-memory setup as the same JSON object written by save() to .ptzc.
         * Compact dump (no indent); save() pretty-prints with indent 2.
         */
        std::string toJsonString(const CameraManager& mgr, const SceneStore& scenes) const;

        /**
         * Saves the current presets and aliases (if set) for all cameras
         * in the manager to the given file.
         * If filename has no path component, it is resolved relative to the
         * user-writable app data directory (%LOCALAPPDATA%\PTZCommander).
         * If filename has no '.', ".ptzc" is appended.
         * Returns true on success.
         */
        bool save(const CameraManager& mgr, const SceneStore& scenes,
                  std::string filename = "presets.ptzc") const {
            return save(mgr, scenes, {}, {}, filename);
        }

        bool save(const CameraManager& mgr, const SceneStore& scenes,
                  const std::vector<visca::ViscaIpEndpoint>& viscaIp,
                  const std::vector<visca::ViscaSerialEndpoint>& viscaSerial,
                  std::string filename = "presets.ptzc") const;

        /**
         * Normalizes a filename for .ptzc files: appends ".ptzc" if the path has no extension.
         * (Path resolution for user data is handled separately in load/save.)
         */
        static std::string normalizeFilename(std::string filename);

        /**
         * User-writable directory for bare preset filenames
         * (%LOCALAPPDATA%\PTZCommander, or "." if LOCALAPPDATA is unset).
         */
        static std::string homeDirectory();

        /**
         * Filenames of *.ptzc files in homeDirectory(), sorted.
         * Missing/unreadable directories yield an empty list.
         */
        static std::vector<std::string> listHomeFiles();

    private:
        static std::string getDefaultPresetDir();
        static std::string resolvePresetPath(std::string filename);

        // Internal helpers could go here for future backends
    };

} // namespace cameras

#endif // CAMERAS_PRESETSTORE_H
