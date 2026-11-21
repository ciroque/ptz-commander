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

        // Keeps the camera, its presets, and this connection record without
        // probing. The stored password is cleared. A dpapi blob this user
        // cannot decrypt is kept. The camera is not connected.
        OnvifAddResult addInactive(const OnvifEndpoint& endpoint);

        // Sets the password on a camera already in the manager, probes, and
        // on success stores the password for the next setup save.
        OnvifAddResult login(const std::string& serial, const std::string& password);

        const std::vector<OnvifEndpoint>& endpoints() const { return endpoints_; }

    private:
        CameraManager& manager_;
        std::vector<OnvifEndpoint> endpoints_;
    };

} // namespace cameras::onvif

#endif
