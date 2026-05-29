#include "TinyFamilyStrategy.h"
#include <algorithm>

namespace cameras::obsbot::strategies {

    namespace {
        template<typename T>
        T clamp(const T& val, const T& minVal, const T& maxVal) {
            return std::min(std::max(val, minVal), maxVal);
        }
    }

    bool TinyFamilyStrategy::disableAI(Device* dev) {
        if (!dev) return false;

        int32_t aiResult = dev->cameraSetAiModeU(Device::AiWorkModeNone, 0);
        int32_t bootResult = dev->cameraSetBootModeU(Device::AiWorkModeNone, Device::AiSubModeNormal);

        dev->aiSetEnabledR(false);
        for (int i = 0; i < 5; ++i) {
            dev->aiSetGestureCtrlIndividualR(i, false);
        }

        return (aiResult == RM_RET_OK && bootResult == RM_RET_OK);
    }

    bool TinyFamilyStrategy::moveTo(float pan, float tilt, int zoom, Device* dev) {
        if (!dev) return false;

        zoom = clamp(zoom, 0, 100);
        float scaledZoom = 1.0f + (zoom / 100.0f);
        int32_t zoomResult = dev->cameraSetZoomAbsoluteR(scaledZoom);

        // Tiny 2 prefers absolute angle, others use speed position
        int32_t moveResult;
        if (dev->productType() == ObsbotProdTiny2) {
            // SDK signature: aiSetGimbalMotorAngleR(float pitch, float yaw, float roll)
            // pitch = tilt, yaw = pan. We pass roll=0 explicitly.
            moveResult = dev->aiSetGimbalMotorAngleR(tilt, pan, 0.0f);
        }
        else {
            moveResult = dev->gimbalSetSpeedPositionR(0, tilt, pan, 80, 80, 80);
        }

        return (zoomResult == RM_RET_OK && moveResult == RM_RET_OK);
    }

    bool TinyFamilyStrategy::setZoom(int zoom, int speed, Device* dev) {
        if (!dev) return false;

        zoom = clamp(zoom, 0, 100);
        uint32_t zoomRatio = static_cast<uint32_t>(100 + (zoom * 3));

        uint32_t zoomSpeed;
        if (speed == 255) {
            zoomSpeed = 255u;
        }
        else {
            int scaledSpeed = clamp((speed * 10) / 100, 0, 10);
            zoomSpeed = (speed > 0 && scaledSpeed == 0) ? 1u : static_cast<uint32_t>(scaledSpeed);
        }

        return dev->cameraSetZoomWithSpeedAbsoluteR(zoomRatio, zoomSpeed) == RM_RET_OK;
    }

} // namespace cameras::obsbot::strategies