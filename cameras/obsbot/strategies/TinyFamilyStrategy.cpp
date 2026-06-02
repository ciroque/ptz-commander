#include "TinyFamilyStrategy.h"
#include "StrategyUtils.h"

namespace cameras::obsbot::strategies {

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

        float scaledZoom = scaleZoomForAbsoluteSet(zoom);
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

        uint32_t zoomRatio = computeZoomRatio(zoom);
        uint32_t zoomSpeed = computeZoomSpeed(speed);

        return dev->cameraSetZoomWithSpeedAbsoluteR(zoomRatio, zoomSpeed) == RM_RET_OK;
    }

    cameras::Ptz TinyFamilyStrategy::getCurrentPtz(Device* dev) {
        return readCommonPtzFromDevice(dev);
    }

} // namespace cameras::obsbot::strategies