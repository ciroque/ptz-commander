#include "TailAirStrategy.h"
#include "StrategyUtils.h"

namespace cameras::obsbot::strategies {

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

        float scaledZoom = scaleZoomForAbsoluteSet(zoom);
        int32_t zoomResult = dev->cameraSetZoomAbsoluteR(scaledZoom);

        // SDK signature: aiSetGimbalMotorAngleR(float pitch, float yaw, float roll)
        // pitch = tilt, yaw = pan. We pass roll=0 explicitly.
        int32_t gimbalResult = dev->aiSetGimbalMotorAngleR(tilt, pan, 0.0f);

        return zoomResult == RM_RET_OK && gimbalResult == RM_RET_OK;
    }

    bool TailAirStrategy::setZoom(int zoom, int speed, Device* dev) {
        if (!dev) return false;

        uint32_t zoomRatio = computeZoomRatio(zoom);
        uint32_t zoomSpeed = computeZoomSpeed(speed);

        return dev->cameraSetZoomWithSpeedAbsoluteR(zoomRatio, zoomSpeed) == RM_RET_OK;
    }

    cameras::Ptz TailAirStrategy::getCurrentPtz(Device* dev) {
        return readCommonPtzFromDevice(dev);
    }

} // namespace cameras::obsbot::strategies