#include "ObsbotCamera.h"

#include <iostream>
#include <thread>

namespace cameras::obsbot {
    ObsbotCamera::ObsbotCamera(std::shared_ptr<Device> dev) : device_(std::move(dev)) {}

    std::string ObsbotCamera::getSerialNumber() const 
    {
        return device_ ? device_->devSn() : "Unknown";
    }

    std::string ObsbotCamera::getName() const 
    {
        return device_ ? device_->devName() : "Unknown";
    }

    bool ObsbotCamera::setPosition(float pan, float pitch, float zoom) const
    {
        //std::cout << "pan: " << pan << ", pitch: " << pitch << ", zoom: " << zoom << std::endl;

        if (device_) {
            int32_t result;

            result = device_->cameraSetZoomAbsoluteR(zoom);
            //std::cout << "device_->cameraSetZoomAbsoluteR(" << zoom << ") = " << result << std::endl;

            result = device_->gimbalSetSpeedPositionR(RollValue, pitch, pan, MaxMoveSpeed, MaxMoveSpeed, MaxMoveSpeed);
            //std::cout << "device_->gimbalSetSpeedPositionR = " << result << std::endl;
        }

        return true;
    }

    CameraState ObsbotCamera::getCurrentState() const
    {
        auto cameraState = CameraState();
        if (device_) {
            int32_t result;
            float currentPos[3];

            result = device_->cameraGetZoomAbsoluteR(cameraState.zoom);

            result = device_->gimbalGetAttitudeInfoR(currentPos);

            cameraState.roll = currentPos[0];
            cameraState.pitch = currentPos[1];
            cameraState.pan = currentPos[2];
        }

        return cameraState;
    }

    bool ObsbotCamera::isConnected() const {
        return device_ != nullptr;  // Simple check; could refine with SDK status if needed
    }
}