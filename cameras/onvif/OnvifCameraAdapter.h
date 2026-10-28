#ifndef CAMERAS_ONVIF_ONVIF_CAMERA_ADAPTER_H
#define CAMERAS_ONVIF_ONVIF_CAMERA_ADAPTER_H

#include "../CameraManager.h"
#include "OnvifEndpoint.h"

#include <vector>

namespace cameras::onvif {

class OnvifCameraAdapter {
public:
    explicit OnvifCameraAdapter(CameraManager& manager);

    OnvifAddResult add(const OnvifEndpoint& endpoint);
    const std::vector<OnvifEndpoint>& endpoints() const { return endpoints_; }

private:
    CameraManager& manager_;
    std::vector<OnvifEndpoint> endpoints_;
};

} // namespace cameras::onvif

#endif
