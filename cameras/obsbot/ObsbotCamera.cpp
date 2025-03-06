#include "ObsbotCamera.h"

namespace cameras {
    ObsbotCamera::ObsbotCamera(std::shared_ptr<Device> dev) : device_(std::move(dev)) {
        if (device_) {
            // Disable AI on creation
            int32_t setAiModeResult = device_->cameraSetAiModeU(Device::AiWorkModeNone, 0);
            int32_t setBootModeResult = device_->cameraSetBootModeU(Device::AiWorkModeNone, Device::AiSubModeNormal);

            // I think this is all that is needed to disable the AI stuff, but going to leave the rest just in case!
            device_->aiSetEnabledR(false);

            for (int i = 0; i < 5; i++) {
				device_->aiSetGestureCtrlIndividualR(i, false);
            }

            if (setAiModeResult != RM_RET_OK || setBootModeResult != RM_RET_OK) {
                std::cout << "Warning: Failed to disable AI mode for " << getSerialNumber()
                    << " (AI: " << setAiModeResult << ", Boot: " << setBootModeResult << ")" << std::endl;
            }
        }
    }

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
        zoom = zoom < 0 ? 0 : zoom > 100 ? 100 : zoom;
        float scaledZoom = 1.0f + (zoom / 100.0f);
        int32_t zoomResult = device_->cameraSetZoomAbsoluteR(scaledZoom);
        int32_t gimbalResult = device_->gimbalSetSpeedPositionR(RollValue, tilt, pan, MaxMoveSpeed, MaxMoveSpeed, MaxMoveSpeed);
        return (zoomResult == 0 && gimbalResult == 0);
    }

    Ptz ObsbotCamera::getCurrentPtz() const {
        Ptz ptz{ 0.0f, 0.0f, 0 }; 
        if (device_) {
            float pos[3];
            if (device_->gimbalGetAttitudeInfoR(pos) == 0) {
                ptz.tilt = pos[1];  
                ptz.pan = pos[2];   
            }
            float zoom = 0.0f;
            if (device_->cameraGetZoomAbsoluteR(zoom) == 0) {
                ptz.zoom = (zoom - 1.0) * 100;
            }
        }
        return ptz;
    }
}