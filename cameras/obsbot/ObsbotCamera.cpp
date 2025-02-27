#include "ObsbotCamera.h"

namespace cameras {
    ObsbotCamera::ObsbotCamera(std::shared_ptr<Device> dev) : device_(std::move(dev)) {}

    std::string ObsbotCamera::getSerialNumber() const {
        return device_ ? device_->devSn() : "Unknown";
    }

    std::string ObsbotCamera::getName() const {
        return device_ ? device_->devName() : "Unknown";
    }

    bool ObsbotCamera::isConnected() const {
        return device_ != nullptr;  // Could refine with SDK status
    }

    bool ObsbotCamera::setPosition(float pan, float tilt, float zoom) {
        if (!device_) return false;

        int32_t zoomResult = device_->cameraSetZoomAbsoluteR(zoom);
        int32_t gimbalResult = device_->gimbalSetSpeedPositionR(RollValue, tilt, pan, MaxMoveSpeed, MaxMoveSpeed, MaxMoveSpeed);
        return (zoomResult == 0 && gimbalResult == 0);  // True if both succeed
    }

    Preset ObsbotCamera::getCurrentState() const {
        Preset state;
        if (device_) {
            float pos[3];
            if (device_->gimbalGetAttitudeInfoR(pos) == 0) {
                state.roll = pos[0];  
                state.pitch = pos[1]; 
                state.pan = pos[2];   
            }
            device_->cameraGetZoomAbsoluteR(state.zoom);
        }
        return state;
    }
}
