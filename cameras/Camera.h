#ifndef CAMERAS_CAMERA_H
#define CAMERAS_CAMERA_H

#include "Preset.h"
#include <string>
#include <list>
#include <memory>

namespace cameras {
    class Camera {
    protected:
        std::list<std::shared_ptr<Preset>> presets_;  // Preset storage

    public:
        virtual ~Camera() = default;

        // Core PTZ methods (still virtual for overrides)
        virtual std::string getSerialNumber() const = 0;
        virtual std::string getName() const = 0;
        virtual bool isConnected() const = 0;
        virtual bool setPosition(float pan, float tilt, int zoom) = 0;
        virtual Ptz getCurrentPtz() const = 0;

        // Preset methods (concrete in base class)
        std::list<std::shared_ptr<Preset>> getPresets() const {
            return presets_;  // Returns copy—thread-safe via CameraManager mutex
        }

        void AddPreset(Preset state) {
            presets_.push_back(std::make_shared<Preset>(state));
        }
    };
}

#endif // CAMERAS_CAMERA_H