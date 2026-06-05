#include "ViscaCameraAdapter.h"

#include "ViscaCamera.h"
#include "ViscaDiscovery.h"
#include "../../core/Logger.h"

#include <algorithm>
#include <cctype>
#include <chrono>

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
    if (!running_) return;
    running_ = false;

    if (worker_.joinable()) {
        worker_.join();
    }

    core::Logger::info("ViscaCameraAdapter stopped");
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

} // namespace cameras::visca
