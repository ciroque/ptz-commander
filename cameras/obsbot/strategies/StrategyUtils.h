#pragma once

#include <algorithm>
#include "../../../include/obsbot/dev.hpp"
#include "../../Preset.h"

namespace cameras::obsbot::strategies {

    /**
     * Shared clamp helper (previously duplicated in each strategy .cpp).
     */
    template<typename T>
    inline T clamp(const T& val, const T& minVal, const T& maxVal) {
        return std::min(std::max(val, minVal), maxVal);
    }

    // --- Zoom scaling helpers (common to both strategies) ---

    /**
     * Scales 0-100 zoom to the absolute zoom value expected by cameraSetZoomAbsoluteR.
     * Used in moveTo() for both Tiny and TailAir.
     */
    inline float scaleZoomForAbsoluteSet(int zoom) {
        zoom = clamp(zoom, 0, 100);
        return 1.0f + (static_cast<float>(zoom) / 100.0f);
    }

    /**
     * Computes the zoomRatio for cameraSetZoomWithSpeedAbsoluteR.
     */
    inline uint32_t computeZoomRatio(int zoom) {
        zoom = clamp(zoom, 0, 100);
        return static_cast<uint32_t>(100 + (zoom * 3));
    }

    /**
     * Computes the speed value for cameraSetZoomWithSpeedAbsoluteR.
     */
    inline uint32_t computeZoomSpeed(int speed) {
        if (speed == 255) {
            return 255u;
        }
        int scaledSpeed = clamp((speed * 10) / 100, 0, 10);
        return (speed > 0 && scaledSpeed == 0) ? 1u : static_cast<uint32_t>(scaledSpeed);
    }

    /**
     * Converts the value from cameraGetZoomAbsoluteR back to 0-100 range.
     * Used in getCurrentPtz().
     */
    inline int scaleZoomFromAbsoluteRead(float zoomVal) {
        return static_cast<int>((zoomVal - 1.0f) * 100.0f);
    }

    /**
     * Common PTZ reading logic using gimbalGetAttitudeInfoR + cameraGetZoomAbsoluteR.
     * Currently identical for Tiny and TailAir families; placed here so it can be
     * customized per-strategy later if the SDK reports differ (e.g. different pos indices).
     */
    inline cameras::Ptz readCommonPtzFromDevice(Device* dev) {
        cameras::Ptz ptz{ 0.0f, 0.0f, 0 };
        if (!dev) return ptz;

        float pos[3] = { 0 };
        if (dev->gimbalGetAttitudeInfoR(pos) == RM_RET_OK) {
            ptz.tilt = pos[1];
            ptz.pan = pos[2];
        }

        float zoomVal = 0.0f;
        if (dev->cameraGetZoomAbsoluteR(zoomVal) == RM_RET_OK) {
            ptz.zoom = scaleZoomFromAbsoluteRead(zoomVal);
        }
        return ptz;
    }

} // namespace cameras::obsbot::strategies
