#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include "../cameras/visca/ViscaCalibration.h"
#include "../cameras/visca/ViscaIpEndpoint.h"
#include "../cameras/visca/ViscaSerialEndpoint.h"
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace data {
    class Context {
    public:
        cameras::CameraManager& cameraMgr;
        cameras::SceneStore& sceneStore;
        std::ostream& out;
        std::ostream& err;
        std::function<void()> onSetupChanged;
        // False with error set if the camera can't be added.
        std::function<bool(const cameras::visca::ViscaIpEndpoint&, std::string& error)> addViscaIp;
        // Add, or move an existing camera onto the endpoint's profile (setup load).
        std::function<bool(const cameras::visca::ViscaIpEndpoint&)> loadViscaIp;
        // Calibrates and applies the profile; nullopt with error set on failure.
        std::function<std::optional<cameras::visca::ViscaProfile>(
            const std::string& serial,
            const cameras::visca::CalibrationRanges&,
            const cameras::visca::ViscaCalibrator::Progress&,
            std::string& error)> calibrateViscaIp;
        std::vector<cameras::visca::ViscaIpEndpoint> viscaIpEndpoints;
        // The same for VISCA cameras on COM ports.
        std::function<bool(const cameras::visca::ViscaSerialEndpoint&, std::string& error)> addViscaSerial;
        std::function<bool(const cameras::visca::ViscaSerialEndpoint&)> loadViscaSerial;
        std::vector<cameras::visca::ViscaSerialEndpoint> viscaSerialEndpoints;
        // Rows of the last `camera scan visca-ip` or `camera scan serial`
        // (no profile chosen yet), for `camera add scanned <n>`; nullopt
        // until a scan has run. A new scan of either kind replaces them.
        using ScannedCamera = std::variant<cameras::visca::ViscaIpEndpoint, cameras::visca::ViscaSerialEndpoint>;
        std::optional<std::vector<ScannedCamera>> lastScan;

        Context(cameras::CameraManager& mgr,
                cameras::SceneStore& scenes,
                std::ostream& output = std::cout,
                std::ostream& error = std::cerr)
            : cameraMgr(mgr), sceneStore(scenes), out(output), err(error) {}

        void notifySetupChanged() {
            if (onSetupChanged) {
                onSetupChanged();
            }
        }
    };
}

#endif // DATA_CONTEXT_H