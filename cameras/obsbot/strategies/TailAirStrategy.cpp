#include "TailAirStrategy.h"
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

        // SDK signature: aiSetGimbalMotorAngleR(float pitch, float yaw, float roll)
        // pitch = tilt, yaw = pan. We pass roll=0 explicitly.
        int32_t gimbalResult = dev->aiSetGimbalMotorAngleR(tilt, pan, 0.0f);

        return zoomResult == RM_RET_OK && gimbalResult == RM_RET_OK;
    }

    bool TailAirStrategy::setZoom(int zoom, int speed, Device* dev) {
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

    cameras::Ptz TailAirStrategy::getCurrentPtz(Device* dev) {
        cameras::Ptz ptz{ 0.0f, 0.0f, 0 };
        if (!dev) return ptz;

        float pos[3] = { 0 };
        if (dev->gimbalGetAttitudeInfoR(pos) == RM_RET_OK) {
            ptz.tilt = pos[1];
            ptz.pan = pos[2];
        }

        float zoomVal = 0.0f;
        if (dev->cameraGetZoomAbsoluteR(zoomVal) == RM_RET_OK) {
            ptz.zoom = static_cast<int>((zoomVal - 1.0f) * 100.0f);
        }
        return ptz;
    }

} // namespace cameras::obsbot::strategies