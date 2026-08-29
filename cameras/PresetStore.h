#ifndef CAMERAS_PRESETSTORE_H
#define CAMERAS_PRESETSTORE_H

#include "CameraManager.h"
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
     *   ...
     * }
     *
     * This is the central abstraction for preset file persistence.
     */
    enum class LoadStatus {
        Ok,
        NotFound,
        ParseError
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
         * File contents are fully parsed before any camera is mutated; a parse
         * failure leaves in-memory state unchanged.
         */
        LoadStatus load(CameraManager& mgr, std::string filename = "presets.ptzc");

        /**
         * Saves the current presets and aliases (if set) for all cameras
         * in the manager to the given file.
         * If filename has no path component, it is resolved relative to the
         * user-writable app data directory (%LOCALAPPDATA%\PTZCommander).
         * If filename has no '.', ".ptzc" is appended.
         * Returns true on success.
         */
        bool save(const CameraManager& mgr, std::string filename = "presets.ptzc") const;

        /**
         * Normalizes a filename for .ptzc files: appends ".ptzc" if no extension dot is present.
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
