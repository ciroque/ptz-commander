#ifndef CAMERAS_ABSENTOBSBOT_H
#define CAMERAS_ABSENTOBSBOT_H

#include "CameraManager.h"
#include "PresetStore.h"

#include <memory>
#include <string>

namespace cameras {

    // An OBSBOT serial that is in the setup but not plugged in. It stays in
    // the manager so camera list, setup save, and camera discard can see it.
    // No OBSBOT SDK: tests build this on its own.
    class AbsentObsbotCamera : public Camera {
    public:
        explicit AbsentObsbotCamera(std::string serial);

        std::string getSerialNumber() const override;
        std::string getName() const override;
        bool isConnected() const override;
        ConnectionState connectionState() const override;

        bool setPosition(float pan, float tilt, int zoom) override;
        bool setZoom(int zoom, int speed) override;
        Ptz getCurrentPtz() const override;

    private:
        std::string serial_;
    };

    // OBSBOT serials are device serial numbers. The other kinds carry a prefix.
    bool isObsbotSerial(const std::string& serial);

    // After setup load has applied presets to cameras that are already
    // present, add a placeholder for each staged OBSBOT serial that is not.
    void retainUnpluggedObsbot(CameraManager& mgr, const StagedSetup& staged);

    // Unplug: copy alias and presets onto a placeholder, then swap it in.
    // A serial that is already a placeholder is left alone.
    void shelveObsbot(CameraManager& mgr, const std::string& serial);

    // Plug-in: copy a placeholder onto this camera, then swap the live camera
    // in. Any Camera can receive the copy. A live camera already in the
    // manager is left as it is.
    void restoreObsbot(CameraManager& mgr, const std::shared_ptr<Camera>& live);

} // namespace cameras

#endif // CAMERAS_ABSENTOBSBOT_H
