#ifndef CAMERAS_OBSBOT_OBSBOT_CAMERA_H
#define CAMERAS_OBSBOT_OBSBOT_CAMERA_H

#include "../Camera.h"
#include "../../include/obsbot/comm.hpp"
#include "../../include/obsbot/dev.hpp"
#include "../../include/obsbot/devs.hpp"
#include <memory>
#include <iostream>

namespace cameras {
    class ObsbotCamera : public Camera {
    public:
        explicit ObsbotCamera(std::shared_ptr<Device> dev);
        std::string getSerialNumber() const override;
        std::string getName() const override;
        bool isConnected() const override;
        bool setPosition(float pan, float tilt, int zoom) override;
        Ptz getCurrentPtz() const override;

    private:
        std::shared_ptr<Device> device_;
        static constexpr float RollValue = 0.0f; 
        static constexpr float MaxMoveSpeed = 75.0f;
    };
}

#endif // CAMERAS_OBSBOT_OBSBOT_CAMERA_H
