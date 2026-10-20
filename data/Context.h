#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include "../cameras/visca/ViscaCalibration.h"
#include "../cameras/visca/ViscaIpEndpoint.h"
#include <functional>
#include <iostream>
#include <optional>
#include <string>
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
        // Rows of the last `camera scan visca-ip` (host and port only), for
        // `camera add scanned <n>`; nullopt until a scan has run.
        std::optional<std::vector<cameras::visca::ViscaIpEndpoint>> lastViscaIpScan;

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