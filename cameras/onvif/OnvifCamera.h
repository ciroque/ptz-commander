#ifndef CAMERAS_ONVIF_ONVIF_CAMERA_H
#define CAMERAS_ONVIF_ONVIF_CAMERA_H

#include "../Camera.h"
#include "OnvifClient.h"
#include "OnvifEndpoint.h"

#include <string>

namespace cameras::onvif {

class OnvifCamera : public ::cameras::Camera {
public:
    explicit OnvifCamera(OnvifEndpoint endpoint);

    // GetDeviceInformation, services, and a PTZ profile. Failure leaves the
    // camera unconnected so the caller can refuse to add it.
    bool probe(std::string& error);

    std::string getSerialNumber() const override;
    std::string getName() const override;
    bool isConnected() const override;

    bool setPosition(float pan, float tilt, int zoom) override;
    bool setZoom(int zoom, int speed) override;
    Ptz getCurrentPtz() const override;

private:
    bool moveRaw(double panX, double tiltY, double zoomX, double panTiltSpeed, double zoomSpeed) const;

    OnvifEndpoint endpoint_;
    std::string serial_;
    std::string name_;
    mutable OnvifClient client_;
    mutable Ptz lastPtz_{0.0f, 0.0f, 0};
    mutable OnvifPosition lastRaw_{};
    mutable bool haveRaw_ = false;
    mutable bool connected_ = false;
};

} // namespace cameras::onvif

#endif
