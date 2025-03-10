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

    bool ObsbotCamera::setZoom(int zoom, int speed) {
        if (!device_) return false;

        // Validate inputs
        if (zoom < 0 || zoom > 100 || speed < 0 || speed > 100) return false;

        // Convert zoom (0-100) to scaledZoom (1.0-2.0) like setPosition
        float scaledZoom = 1.0f + (zoom / 100.0f);

        // Convert to zoom_ratio (100x magnification: 1.0-2.0 ? 100-200)
        uint32_t zoomRatio = static_cast<uint32_t>(scaledZoom * 100);

        // Convert speed (0-100%) to zoom_speed (0-10 range, with 255 as max option)
        uint32_t zoomSpeed = (speed * 10) / 100;  // Scale 0-100% to 0-10
        if (zoomSpeed == 0 && speed > 0) zoomSpeed = 1;  // Avoid default (0) unless intentional

        // Set zoom with speed
        int32_t result = device_->cameraSetZoomWithSpeedAbsoluteR(zoomRatio, zoomSpeed);
        return result == RM_RET_OK;
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