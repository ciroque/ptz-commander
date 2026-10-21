#include "ViscaCameraAdapter.h"

#include "ViscaCamera.h"
#include "UdpViscaTransport.h"
#include "ViscaProfile.h"
#include "WindowsSerialTransport.h"
#include "../../core/Logger.h"

#include <algorithm>
#include <memory>

namespace cameras::visca {

ViscaCameraAdapter::ViscaCameraAdapter(CameraManager& manager)
    : manager_(manager)
{
}

namespace {
    std::string ipCameraId(const ViscaIpEndpoint& endpoint) {
        return "VISCA-IP:" + endpoint.host + ":" + std::to_string(endpoint.port);
    }

    ViscaIpEndpoint withDefaults(ViscaIpEndpoint endpoint) {
        if (endpoint.port <= 0) endpoint.port = 52381;
        if (endpoint.profile.empty()) endpoint.profile = "tenveo-30x";
        return endpoint;
    }

    std::optional<ViscaProfile> profileFor(const ViscaIpEndpoint& endpoint) {
        auto profile = resolveProfile(endpoint);
        if (!profile) {
            core::Logger::error("Unknown VISCA profile '" + endpoint.profile + "' for " +
                                ipCameraId(endpoint) + " (known: " + knownProfileNames() + ")");
        }
        return profile;
    }

    std::string profileLabel(const ViscaIpEndpoint& endpoint) {
        return endpoint.profile + (endpoint.calibration ? " (calibrated)" : "");
    }
}

bool ViscaCameraAdapter::addIpCamera(const ViscaIpEndpoint& requested, std::string& error) {
    return createIpCamera(requested, true, error);
}

bool ViscaCameraAdapter::createIpCamera(const ViscaIpEndpoint& requested, bool requireReply, std::string& error) {
    if (requested.host.empty()) {
        error = "no host given";
        return false;
    }
    const auto endpoint = withDefaults(requested);
    const std::string id = ipCameraId(endpoint);
    if (manager_.findBySerial(id)) {
        core::Logger::warn("VISCA-IP camera already present: " + id);
        error = "already added";
        return false;
    }
    const auto profile = profileFor(endpoint);
    if (!profile) {
        error = "unknown profile '" + endpoint.profile + "'";
        return false;
    }

    auto transport = std::make_unique<UdpViscaTransport>(endpoint.host, endpoint.port);
    auto camera = std::make_shared<ViscaCamera>(
        std::move(transport),
        id,
        "VISCA IP (" + endpoint.host + ")",
        static_cast<uint8_t>(1),
        *profile);
    if (requireReply && !camera->isOpen()) {
        error = "could not open UDP to '" + endpoint.host + "'; give an IPv4 address such as 192.168.1.146";
        return false;
    }
    if (requireReply && !camera->answersInquiry()) {
        core::Logger::warn("VISCA-IP camera not added, no reply: " + id);
        error = "no reply to a VISCA inquiry; check the camera is on and the address and port are right";
        return false;
    }
    manager_.addCamera(camera);
    ipEndpoints_.push_back(endpoint);

    core::Logger::info("Added VISCA-IP camera: " + id + " profile=" + profileLabel(endpoint));
    return true;
}

bool ViscaCameraAdapter::loadIpCamera(const ViscaIpEndpoint& requested) {
    if (requested.host.empty()) {
        return false;
    }
    const auto endpoint = withDefaults(requested);
    const std::string id = ipCameraId(endpoint);
    auto camera = std::dynamic_pointer_cast<ViscaCamera>(manager_.findBySerial(id));
    if (!camera) {
        // A saved camera loads even if it's off right now.
        std::string error;
        return createIpCamera(endpoint, false, error);
    }

    // The file's presets were saved under the file's profile, so the camera
    // has to be on that profile before they are loaded.
    const auto profile = profileFor(endpoint);
    if (!profile) {
        return false;
    }
    camera->applyProfile(*profile);
    const auto stored = std::find_if(ipEndpoints_.begin(), ipEndpoints_.end(),
        [&id](const ViscaIpEndpoint& ep) { return ipCameraId(ep) == id; });
    if (stored != ipEndpoints_.end()) {
        *stored = endpoint;
    } else {
        ipEndpoints_.push_back(endpoint);
    }
    core::Logger::info("Updated VISCA-IP camera: " + id + " profile=" + profileLabel(endpoint));
    return true;
}

std::optional<ViscaProfile> ViscaCameraAdapter::calibrateIpCamera(const std::string& serial,
                                                                  const CalibrationRanges& ranges,
                                                                  const ViscaCalibrator::Progress& progress,
                                                                  std::string& error) {
    const auto endpoint = std::find_if(ipEndpoints_.begin(), ipEndpoints_.end(),
        [&serial](const ViscaIpEndpoint& ep) { return ipCameraId(ep) == serial; });
    auto camera = std::dynamic_pointer_cast<ViscaCamera>(manager_.findBySerial(serial));
    if (endpoint == ipEndpoints_.end() || !camera) {
        error = serial + " is not a VISCA-IP camera";
        return std::nullopt;
    }

    auto profile = camera->calibrate(ranges, progress, error);
    if (!profile) {
        return std::nullopt;
    }
    camera->applyProfile(*profile);
    endpoint->calibration = profile;
    return profile;
}

namespace {
    std::optional<ViscaProfile> profileFor(const ViscaSerialEndpoint& endpoint) {
        auto profile = resolveProfile(endpoint);
        if (!profile) {
            core::Logger::error("Unknown VISCA profile '" + endpoint.profile + "' for " +
                                serialCameraId(endpoint) + " (known: " + knownProfileNames() + ")");
        }
        return profile;
    }

    std::string describe(const ViscaSerialEndpoint& endpoint) {
        return serialCameraId(endpoint) + " baud=" + std::to_string(endpoint.baud) + " profile=" + endpoint.profile;
    }
}

bool ViscaCameraAdapter::addSerialCamera(const ViscaSerialEndpoint& endpoint, std::string& error) {
    return createSerialCamera(endpoint, true, error);
}

bool ViscaCameraAdapter::createSerialCamera(const ViscaSerialEndpoint& requested, bool requireReply,
                                            std::string& error) {
    auto endpoint = requested;
    if (endpoint.profile.empty()) endpoint.profile = kSerialDefaultProfile;
    const std::string id = serialCameraId(endpoint);
    if (manager_.findBySerial(id)) {
        error = "already added";
        return false;
    }
    const auto onPort = std::find_if(serialEndpoints_.begin(), serialEndpoints_.end(),
        [&endpoint](const ViscaSerialEndpoint& ep) { return ep.port == endpoint.port; });
    if (onPort != serialEndpoints_.end()) {
        error = endpoint.port + " already has camera " + serialCameraId(*onPort) + " (one camera per port)";
        return false;
    }
    const auto profile = profileFor(endpoint);
    if (!profile) {
        error = "unknown profile '" + endpoint.profile + "'";
        return false;
    }

    auto camera = std::make_shared<ViscaCamera>(
        std::make_unique<WindowsSerialTransport>(endpoint.port, endpoint.baud),
        id,
        "VISCA Serial (" + endpoint.port + " @" + std::to_string(endpoint.baud) + ")",
        static_cast<uint8_t>(endpoint.address),
        *profile);
    if (requireReply && !camera->isOpen()) {
        error = "could not open " + endpoint.port + "; check the adapter is plugged in and no other program is using it";
        return false;
    }
    if (requireReply && !camera->answersInquiry()) {
        core::Logger::warn("VISCA serial camera not added, no reply: " + describe(endpoint));
        error = "no reply to a VISCA inquiry; check the camera is on and the cable, baud rate and address are right";
        return false;
    }
    manager_.addCamera(camera);
    serialEndpoints_.push_back(endpoint);

    core::Logger::info("Added VISCA serial camera: " + describe(endpoint));
    return true;
}

bool ViscaCameraAdapter::loadSerialCamera(const ViscaSerialEndpoint& requested) {
    auto endpoint = requested;
    if (endpoint.profile.empty()) endpoint.profile = kSerialDefaultProfile;
    const std::string id = serialCameraId(endpoint);
    const auto stored = std::find_if(serialEndpoints_.begin(), serialEndpoints_.end(),
        [&id](const ViscaSerialEndpoint& ep) { return serialCameraId(ep) == id; });
    auto camera = std::dynamic_pointer_cast<ViscaCamera>(manager_.findBySerial(id));

    // The port is opened at one rate, so a different rate needs a new camera.
    if (camera && stored != serialEndpoints_.end() && stored->baud != endpoint.baud) {
        manager_.removeCamera(id);
        serialEndpoints_.erase(stored);
        camera.reset();
    }
    if (!camera) {
        // A saved camera loads even if it's off or unplugged right now.
        std::string error;
        if (!createSerialCamera(endpoint, false, error)) {
            core::Logger::error("Could not load VISCA serial camera " + id + ": " + error);
            return false;
        }
        return true;
    }

    // As for VISCA-IP: the file's presets were saved under the file's profile.
    const auto profile = profileFor(endpoint);
    if (!profile) {
        return false;
    }
    camera->applyProfile(*profile);
    if (stored != serialEndpoints_.end()) {
        *stored = endpoint;
    } else {
        serialEndpoints_.push_back(endpoint);
    }
    core::Logger::info("Updated VISCA serial camera: " + describe(endpoint));
    return true;
}

} // namespace cameras::visca
