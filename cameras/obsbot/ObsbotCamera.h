#ifndef CAMERAS_OBSBOT_OBSBOT_CAMERA_H
#define CAMERAS_OBSBOT_OBSBOT_CAMERA_H

#include "../ICamera.h"  // Relative path from cameras/obsbot/

#include "../obsbot/comm.hpp"
#include "../obsbot/dev.hpp"
#include "../obsbot/devs.hpp"

#include "../utils.h"

#include <memory>

namespace cameras::obsbot {  // Nested namespace
    class ObsbotCamera : public cameras::ICamera {
    public:
        static constexpr float MaxMoveSpeed = 90.0;
        static constexpr float RollValue = 0.0;

        explicit ObsbotCamera(std::shared_ptr<Device> dev);
        
        std::string getSerialNumber() const override;
        std::string getName() const override;
        
        bool setPosition(float pan, float tilt, float zoom) const override;
        CameraState getCurrentState() const override;
        
        bool isConnected() const override;

    private:
        std::shared_ptr<Device> device_;
    };
}

#endif // CAMERAS_OBSBOT_OBSBOT_CAMERA_H