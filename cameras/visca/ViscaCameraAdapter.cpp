#include "ViscaCameraAdapter.h"

#include "ViscaCamera.h"
#include "ViscaDiscovery.h"
#include "UdpViscaTransport.h"
#include "ViscaProfile.h"
#include "../../core/Logger.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <memory>

namespace cameras::visca {

ViscaCameraAdapter::ViscaCameraAdapter(CameraManager& manager)
    : manager_(manager)
{
}

ViscaCameraAdapter::~ViscaCameraAdapter() {
    stop();
}

void ViscaCameraAdapter::start() {
    if (running_) return;
    running_ = true;

    core::Logger::info("ViscaCameraAdapter starting (auto-discovery of connected VISCA serial cameras; looks for Keyspan USA-19H or similar by VID/PID or friendly name. Hardware must be plugged in.)");

    // Run discovery on a background thread so it doesn't block startup (mirrors Obsbot pattern)
    worker_ = std::thread([this]() {
        if (running_) {
            discoverAndAdd();
        }
    });
}

void ViscaCameraAdapter::stop() {
    running_ = false;

    if (worker_.joinable()) {
        worker_.join();
        core::Logger::info("ViscaCameraAdapter stopped");
    }
}

void ViscaCameraAdapter::discoverAndAdd() {
    auto candidates = ViscaDiscovery::discover();

    for (const auto& cand : candidates) {
        if (!running_) break;

        // Probe (lightweight for now)
        if (!ViscaDiscovery::probePort(cand)) {
            continue;
        }

        // For automatic discovery we strongly prefer the Keyspan USA-19H.
        // We consider a candidate a match if:
        //  - VID/PID exactly matches, OR
        //  - Description contains "keyspan" or "usa-19" (case insensitive) -- catches cases where VID parse is incomplete.
        // This keeps auto-discovery useful for the target hardware while avoiding random COM ports.
        bool isPreferred = cand.matchesKeyspan;
        if (!isPreferred) {
            std::string descLower = cand.description;
            std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);
            if (descLower.find("keyspan") != std::string::npos ||
                descLower.find("usa-19") != std::string::npos) {
                isPreferred = true;
            }
        }

        if (!isPreferred) {
            core::Logger::info("Skipping non-preferred VISCA candidate (not matching Keyspan VID/PID or 'keyspan'/'usa-19' in name): " + cand.port + " (" + cand.description + ")");
            continue;
        }

        // Create the camera. It will manage its own serial transport.
        auto camera = std::make_shared<ViscaCamera>(cand.port, cand.baud, cand.address);

        // Add to the shared manager (it deduplicates by serial number)
        manager_.addCamera(camera);

        core::Logger::info("Added VISCA serial camera: " + camera->getSerialNumber() +
                           " (" + camera->getName() + ")");

        // For the first working one we can stop if we want "single VISCA" semantics,
        // but allow multiple in case user has several serial VISCA cameras.
        // For now we add all plausible ones.
    }

    core::Logger::info("ViscaCameraAdapter discovery pass complete");
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
    if (requireReply && !camera->isConnected()) {
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

} // namespace cameras::visca
