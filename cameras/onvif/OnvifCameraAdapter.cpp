#include "OnvifCameraAdapter.h"

#include "OnvifCamera.h"
#include "OnvifClient.h"
#include "../../core/Logger.h"

#include <algorithm>
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

    OnvifCameraAdapter::OnvifCameraAdapter(CameraManager& manager) : manager_(manager) {}

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
            result.detail = error.empty() ? ("could not add ONVIF camera " + endpoint.address()) : error;
            result.authRejected = camera->lastError() == OnvifError::AuthRejected;
            core::Logger::warn("ONVIF add failed for " + endpoint.address() + ": " + result.detail);
            return result;
        }

        manager_.addCamera(camera);
        endpoint.label.clear();
        endpoint.preservedPassword.clear();
        endpoints_.push_back(endpoint);
        result.status = OnvifAddStatus::Added;
        core::Logger::info("Added ONVIF camera " + endpoint.serial() + " (" + camera->getName() + ")");
        return result;
    }

    OnvifAddResult OnvifCameraAdapter::addInactive(const OnvifEndpoint& requested) {
        OnvifAddResult result;
        OnvifEndpoint endpoint = normalized(requested);
        endpoint.needsPassword = false;
        endpoint.label.clear();
        if (endpoint.host.empty() || endpoint.user.empty()) {
            result.detail = "host and username are required";
            return result;
        }
        if (manager_.findBySerial(endpoint.serial())) {
            result.status = OnvifAddStatus::AlreadyPresent;
            result.detail = endpoint.serial() + " is already added";
            return result;
        }

        auto camera = std::make_shared<OnvifCamera>(endpoint);
        if (endpoint.passwordRejected) {
            camera->noteAuthRejected();
        }
        endpoint.passwordRejected = false;
        manager_.addCamera(camera);
        endpoints_.push_back(endpoint);
        result.status = OnvifAddStatus::Added;
        core::Logger::info("ONVIF camera " + endpoint.serial() + " left inactive");
        return result;
    }

    OnvifAddResult OnvifCameraAdapter::login(const std::string& serial, const std::string& password) {
        OnvifAddResult result;
        if (password.empty()) {
            result.detail = "a password is required";
            return result;
        }
        auto camera = std::dynamic_pointer_cast<OnvifCamera>(manager_.findBySerial(serial));
        if (!camera) {
            result.detail = serial + " is not an ONVIF camera";
            return result;
        }

        std::string error;
        if (!camera->signIn(password, error)) {
            result.detail = error.empty() ? "could not sign in" : error;
            result.authRejected = camera->lastError() == OnvifError::AuthRejected;
            core::Logger::warn("ONVIF login failed for " + serial + ": " + result.detail);
            return result;
        }

        OnvifEndpoint stored = camera->endpoint();
        stored.preservedPassword.clear();
        const auto it = std::find_if(endpoints_.begin(), endpoints_.end(),
                                     [&stored](const OnvifEndpoint& ep) { return ep.serial() == stored.serial(); });
        if (it == endpoints_.end()) {
            endpoints_.push_back(stored);
        } else {
            *it = stored;
        }
        result.status = OnvifAddStatus::Added;
        core::Logger::info("ONVIF camera " + stored.serial() + " is active");
        return result;
    }

    bool OnvifCameraAdapter::discard(const std::string& serial) {
        const auto it = std::find_if(endpoints_.begin(), endpoints_.end(),
                                     [&serial](const OnvifEndpoint& ep) { return ep.serial() == serial; });
        if (it == endpoints_.end()) {
            return false;
        }
        endpoints_.erase(it);
        manager_.removeCamera(serial);
        return true;
    }

} // namespace cameras::onvif
