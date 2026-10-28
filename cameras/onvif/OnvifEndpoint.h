#ifndef CAMERAS_ONVIF_ONVIF_ENDPOINT_H
#define CAMERAS_ONVIF_ONVIF_ENDPOINT_H

#include <string>
#include <vector>

namespace cameras::onvif {

struct OnvifEndpoint {
    std::string host;
    int port = 80;
    std::string servicePath = "/onvif/device_service";
    std::string user;
    std::string password;
    // Display name from WS-Discovery. Not required to connect, and not saved.
    std::string label;

    std::string address() const {
        return host + ":" + std::to_string(port > 0 ? port : 80);
    }

    std::string serial() const {
        return "ONVIF:" + address();
    }
};

enum class OnvifAddStatus {
    Added,
    AlreadyPresent,
    Failed
};

struct OnvifAddResult {
    OnvifAddStatus status = OnvifAddStatus::Failed;
    std::string detail;
};

struct OnvifScanResult {
    std::vector<OnvifEndpoint> devices;
    std::string error;
};

} // namespace cameras::onvif

#endif
