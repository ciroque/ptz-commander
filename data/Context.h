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
        std::function<bool(const cameras::visca::ViscaIpEndpoint&)> addViscaIp;
        // Calibrates and applies the profile; nullopt with error set on failure.
        std::function<std::optional<cameras::visca::ViscaProfile>(
            const std::string& serial,
            const cameras::visca::CalibrationRanges&,
            const cameras::visca::ViscaCalibrator::Progress&,
            std::string& error)> calibrateViscaIp;
        std::vector<cameras::visca::ViscaIpEndpoint> viscaIpEndpoints;

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