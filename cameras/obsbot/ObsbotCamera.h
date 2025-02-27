#ifndef CAMERAS_OBSBOT_OBSBOT_CAMERA_H
#define CAMERAS_OBSBOT_OBSBOT_CAMERA_H

#include "../Camera.h"  // Now Camera, not ICamera
#include "../obsbot/comm.hpp"
#include "../obsbot/dev.hpp"
#include "../obsbot/devs.hpp"
#include <memory>

namespace cameras {
    class ObsbotCamera : public Camera {  // Inherit from Camera
    public:
        explicit ObsbotCamera(std::shared_ptr<Device> dev);
        std::string getSerialNumber() const override;
        std::string getName() const override;
        bool isConnected() const override;
        bool setPosition(float pan, float tilt, float zoom) override;
        Preset getCurrentPos() const override;

    private:
        std::shared_ptr<Device> device_;
        static constexpr float RollValue = 0.0f;  // Tiny 4K roll unused
        static constexpr float MaxMoveSpeed = 90.0f;  // Default speed
    };
}

#endif // CAMERAS_OBSBOT_OBSBOT_CAMERA_H