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

    bool ObsbotCamera::setPosition(float pan, float tilt, int zoom) {
        if (!device_) return false;

        float scaledZoom = 1.0f + (zoom / 100.0f);

        int32_t zoomResult = device_->cameraSetZoomAbsoluteR(scaledZoom);
        int32_t gimbalResult = device_->gimbalSetSpeedPositionR(RollValue, tilt, pan, MaxMoveSpeed, MaxMoveSpeed, MaxMoveSpeed);
        return (zoomResult == 0 && gimbalResult == 0);  // True if both succeed
    }

    Ptz ObsbotCamera::getCurrentPtz() const {
        Ptz ptz{};
        if (device_) {
            float pos[3];
            if (device_->gimbalGetAttitudeInfoR(pos) == 0) {
                ptz.tilt = pos[1]; 
                ptz.pan = pos[2];   
            }
			float zoom = 0.0f;
            if (device_->cameraGetZoomAbsoluteR(zoom)) {
				ptz.zoom = (zoom - 1.0) * 100;
            }
        }
        return ptz;
    }
}
