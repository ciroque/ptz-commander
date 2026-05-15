#include "ObsbotCamera.h"
#include "../../include/obsbot/dev.hpp"
#include <iostream>
#include <algorithm>   // for std::min / std::max

namespace cameras {

    // C++11 compatible clamp
    template<typename T>
    T clamp(const T& val, const T& minVal, const T& maxVal) {
        return std::min(std::max(val, minVal), maxVal);
    }

    ObsbotCamera::ObsbotCamera(std::shared_ptr<Device> dev)
        : device_(std::move(dev))
    {
        if (!device_) return;

        std::string sn = getSerialNumber();
        std::cout << "Initializing camera: " << getName() << " (" << sn << ")" << std::endl;

        bool disableSuccess = true;

        // === Model-specific AI disable ===
        auto prod = device_->productType();

        if (prod == ObsbotProdTailAir) {
            // Tail Air path
            int32_t r1 = device_->aiSetAiTrackModeEnabledR(Device::AiTrackNormal, false);
            int32_t r2 = device_->aiSetAiTrackModeEnabledR(Device::AiTrackHumanNormal, false);
            int32_t r3 = device_->aiSetAiTrackModeEnabledR(Device::AiTrackGroup, false);

            if (r1 != RM_RET_OK && r2 != RM_RET_OK && r3 != RM_RET_OK) {
                std::cout << "Warning: Failed to disable Tail Air tracking modes for " << sn << std::endl;
                disableSuccess = false;
            }

            // Disable gestures too
            device_->aiSetGestureParaR(Device::DevGestureParaTypeGesture, false);

        }
        else {
            // Tiny / Tiny2 / Tiny4K / Tiny SE path
            int32_t setAiModeResult = device_->cameraSetAiModeU(Device::AiWorkModeNone, 0);
            int32_t setBootModeResult = device_->cameraSetBootModeU(Device::AiWorkModeNone, Device::AiSubModeNormal);

            device_->aiSetEnabledR(false);

            for (int i = 0; i < 5; ++i) {
                device_->aiSetGestureCtrlIndividualR(i, false);
            }

            if (setAiModeResult != RM_RET_OK || setBootModeResult != RM_RET_OK) {
                std::cout << "Warning: Failed to disable AI mode for " << sn
                    << " (AI: " << setAiModeResult << ", Boot: " << setBootModeResult << ")" << std::endl;
                disableSuccess = false;
            }
        }

        if (disableSuccess) {
            std::cout << "✅ AI / Tracking fully disabled for " << sn << std::endl;
        }
    }

    std::string ObsbotCamera::getSerialNumber() const {
        return device_ ? device_->devSn() : "Unknown";
    }

    std::string ObsbotCamera::getName() const {
        return device_ ? device_->devName() : "Unknown";
    }

    bool ObsbotCamera::isConnected() const {
        return device_ != nullptr;
    }

    bool ObsbotCamera::setPosition(float pan, float tilt, int zoom) {
        if (!device_) return false;

        zoom = clamp(zoom, 0, 100);
        float scaledZoom = 1.0f + (zoom / 100.0f);

        int32_t zoomResult = device_->cameraSetZoomAbsoluteR(scaledZoom);
        int32_t gimbalResult = device_->gimbalSetSpeedPositionR(0, tilt, pan, 100, 100, 100); // Roll=0

        return (zoomResult == RM_RET_OK && gimbalResult == RM_RET_OK);
    }

    bool ObsbotCamera::setZoom(int zoom, int speed) {
        if (!device_) return false;

        zoom = clamp(zoom, 0, 100);
        uint32_t zoomRatio = static_cast<uint32_t>(100 + (zoom * 3));
        uint32_t zoomSpeed = (speed == 255) ? 255 : static_cast<uint32_t>(clamp((speed * 10) / 100, 0, 10));

        int32_t result = device_->cameraSetZoomWithSpeedAbsoluteR(zoomRatio, zoomSpeed);
        return result == RM_RET_OK;
    }

    Ptz ObsbotCamera::getCurrentPtz() const {
        Ptz ptz{ 0.0f, 0.0f, 0 };
        if (!device_) return ptz;

        float pos[3] = { 0 };
        if (device_->gimbalGetAttitudeInfoR(pos) == RM_RET_OK) {
            ptz.tilt = pos[1];
            ptz.pan = pos[2];
        }

        float zoom = 0.0f;
        if (device_->cameraGetZoomAbsoluteR(zoom) == RM_RET_OK) {
            ptz.zoom = static_cast<int>((zoom - 1.0f) * 100.0f);
        }
        return ptz;
    }

} // namespace cameras