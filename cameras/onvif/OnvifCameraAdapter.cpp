#include "OnvifCameraAdapter.h"

#include "OnvifCamera.h"
#include "../../core/Logger.h"

#include <memory>

namespace cameras::onvif {

namespace {

OnvifEndpoint normalized(OnvifEndpoint endpoint) {
    if (endpoint.port <= 0 || endpoint.port > 65535) {
        endpoint.port = 80;
    }
    if (endpoint.servicePath.empty()) {
        endpoint.servicePath = "/onvif/device_service";
    }
    if (endpoint.servicePath.front() != '/') {
        endpoint.servicePath.insert(endpoint.servicePath.begin(), '/');
    }
    return endpoint;
}

} // namespace

OnvifCameraAdapter::OnvifCameraAdapter(CameraManager& manager)
    : manager_(manager) {}

OnvifAddResult OnvifCameraAdapter::add(const OnvifEndpoint& requested) {
    OnvifAddResult result;
    OnvifEndpoint endpoint = normalized(requested);
    if (endpoint.host.empty() || endpoint.user.empty() || endpoint.password.empty()) {
        result.detail = "host, username, and password are required";
        return result;
    }
    if (manager_.findBySerial(endpoint.serial())) {
        result.status = OnvifAddStatus::AlreadyPresent;
        result.detail = endpoint.serial() + " is already added";
        return result;
    }

    auto camera = std::make_shared<OnvifCamera>(endpoint);
    std::string error;
    if (!camera->probe(error)) {
        result.detail = error.empty()
            ? ("could not add ONVIF camera " + endpoint.address())
            : error;
        core::Logger::warn("ONVIF add failed for " + endpoint.address() + ": " + result.detail);
        return result;
    }

    manager_.addCamera(camera);
    endpoint.label.clear();
    endpoints_.push_back(endpoint);
    result.status = OnvifAddStatus::Added;
    core::Logger::info("Added ONVIF camera " + endpoint.serial() + " (" + camera->getName() + ")");
    return result;
}

} // namespace cameras::onvif
