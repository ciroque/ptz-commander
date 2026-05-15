#include "TailAirStrategy.h"
#include <iostream>
#include <algorithm>

namespace cameras::obsbot::strategies {

    namespace {
        template<typename T>
        T clamp(const T& val, const T& minVal, const T& maxVal) {
            return std::min(std::max(val, minVal), maxVal);
        }
    }

    bool TailAirStrategy::disableAI(Device* dev) {
        if (!dev) return false;

        int32_t r1 = dev->aiSetAiTrackModeEnabledR(Device::AiTrackNormal, false);
        int32_t r2 = dev->aiSetAiTrackModeEnabledR(Device::AiTrackHumanNormal, false);
        int32_t r3 = dev->aiSetAiTrackModeEnabledR(Device::AiTrackGroup, false);
        int32_t gestureResult = dev->aiSetGestureParaR(Device::DevGestureParaTypeGesture, false);

        return (r1 == RM_RET_OK &&
                r2 == RM_RET_OK &&
                r3 == RM_RET_OK &&
                gestureResult == RM_RET_OK);
    }

    bool TailAirStrategy::moveTo(float pan, float tilt, int zoom, Device* dev) {
        if (!dev) return false;

        zoom = clamp(zoom, 0, 100);
        float scaledZoom = 1.0f + (zoom / 100.0f);
        int32_t zoomResult = dev->cameraSetZoomAbsoluteR(scaledZoom);
        int32_t gimbalResult = dev->aiSetGimbalMotorAngleR(0.0f, tilt, pan);

        return zoomResult == RM_RET_OK && gimbalResult == RM_RET_OK;
    }

    bool TailAirStrategy::setZoom(int zoom, int speed, Device* dev) {
        if (!dev) return false;

        zoom = clamp(zoom, 0, 100);
        uint32_t zoomRatio = static_cast<uint32_t>(100 + (zoom * 3));
        uint32_t zoomSpeed = (speed == 255) ? 255u : static_cast<uint32_t>(clamp((speed * 10) / 100, 0, 10));

        return dev->cameraSetZoomWithSpeedAbsoluteR(zoomRatio, zoomSpeed) == RM_RET_OK;
    }

} // namespace cameras::obsbot::strategies