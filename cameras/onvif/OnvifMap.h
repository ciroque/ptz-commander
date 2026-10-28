#ifndef CAMERAS_ONVIF_ONVIF_MAP_H
#define CAMERAS_ONVIF_ONVIF_MAP_H

#include <cmath>

namespace cameras::onvif {

// PositionGenericSpace on the IP4M-1083EW-AI (continuous 360° pan).
// Pan x -1 and +1 are the same heading. App pan is degrees in (-180, 180].
// Tilt y -1..1 spans the mechanical tilt of this class, -15° to +90°.
// Zoom x 0 is wide and 1 is telephoto. App zoom is 0..100.

inline constexpr float kTiltMinDeg = -15.0f;
inline constexpr float kTiltMaxDeg = 90.0f;

inline float wrapPanDegrees(float pan) {
    double x = std::fmod(static_cast<double>(pan), 360.0);
    if (x < 0.0) {
        x += 360.0;
    }
    if (x > 180.0) {
        x -= 360.0;
    }
    if (x <= -180.0) {
        x = 180.0;
    }
    return static_cast<float>(x);
}

inline double panDegreesToX(float pan) {
    const float wrapped = wrapPanDegrees(pan);
    if (wrapped >= 180.0f) {
        return 1.0;
    }
    return static_cast<double>(wrapped) / 180.0;
}

inline float xToPanDegrees(double x) {
    if (x >= 1.0 || x <= -1.0) {
        return 180.0f;
    }
    return static_cast<float>(x * 180.0);
}

inline double tiltDegreesToY(float tilt) {
    double t = static_cast<double>(tilt);
    if (t < kTiltMinDeg) {
        t = kTiltMinDeg;
    }
    if (t > kTiltMaxDeg) {
        t = kTiltMaxDeg;
    }
    return -1.0 + 2.0 * (t - kTiltMinDeg) / (kTiltMaxDeg - kTiltMinDeg);
}

inline float yToTiltDegrees(double y) {
    if (y < -1.0) {
        y = -1.0;
    }
    if (y > 1.0) {
        y = 1.0;
    }
    return static_cast<float>(kTiltMinDeg + (y + 1.0) / 2.0 * (kTiltMaxDeg - kTiltMinDeg));
}

inline double zoomPercentToX(int zoom) {
    if (zoom < 0) {
        zoom = 0;
    }
    if (zoom > 100) {
        zoom = 100;
    }
    return static_cast<double>(zoom) / 100.0;
}

inline int xToZoomPercent(double x) {
    if (x < 0.0) {
        x = 0.0;
    }
    if (x > 1.0) {
        x = 1.0;
    }
    return static_cast<int>(std::lround(x * 100.0));
}

} // namespace cameras::onvif

#endif
