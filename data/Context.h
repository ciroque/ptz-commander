#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include "../cameras/onvif/OnvifEndpoint.h"
#include "../cameras/visca/ViscaCalibration.h"
#include "../cameras/visca/ViscaIpEndpoint.h"
#include "../cameras/visca/ViscaSerialEndpoint.h"
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace commands {
    class TourRunner;
}

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
        // Calibrates a VISCA camera (IP or serial) and applies the profile;
        // nullopt with error set on failure.
        std::function<std::optional<cameras::visca::ViscaProfile>(
            const std::string& serial, const cameras::visca::CalibrationRanges&,
            const cameras::visca::ViscaCalibrator::Progress&, std::string& error)>
            calibrateVisca;
        std::vector<cameras::visca::ViscaIpEndpoint> viscaIpEndpoints;
        // The same for VISCA cameras on COM ports.
        std::function<bool(const cameras::visca::ViscaSerialEndpoint&, std::string& error)> addViscaSerial;
        std::function<bool(const cameras::visca::ViscaSerialEndpoint&)> loadViscaSerial;
        std::vector<cameras::visca::ViscaSerialEndpoint> viscaSerialEndpoints;
        // ONVIF absolute move. Failed leaves detail set; AlreadyPresent adds nothing.
        // authRejected is set when the camera refused the username or password.
        std::function<cameras::onvif::OnvifAddResult(const cameras::onvif::OnvifEndpoint&)> addOnvif;
        // Adds an ONVIF camera that stays in the setup without a password and
        // without a connection. Presets and scene bindings can still attach.
        std::function<cameras::onvif::OnvifAddResult(const cameras::onvif::OnvifEndpoint&)> addInactiveOnvif;
        // Sets the password on an ONVIF camera already in the manager and connects.
        std::function<cameras::onvif::OnvifAddResult(const std::string& serial, const std::string& password)>
            loginOnvif;
        // Removes one camera from its adapter and from the manager, then
        // copies the adapter lists back onto the endpoint vectors. commit
        // false only checks: nothing is removed, and Removed means it can
        // be discarded. Obsbot means the camera is still plugged in and
        // nothing was removed.
        enum class DiscardCameraStatus { Removed, NotFound, Obsbot };
        std::function<DiscardCameraStatus(const std::string& serial, bool commit)> discardCamera;
        std::vector<cameras::onvif::OnvifEndpoint> onvifEndpoints;
        // Rows of the last `camera scan` (no VISCA profile, and no ONVIF
        // user or password yet), for `camera add scanned <n>`. nullopt until
        // a scan has run. A new scan replaces them.
        using ScannedCamera = std::variant<cameras::visca::ViscaIpEndpoint, cameras::visca::ViscaSerialEndpoint,
                                           cameras::onvif::OnvifEndpoint>;
        std::optional<std::vector<ScannedCamera>> lastScan;
        // Null keeps scene tour, preset tour, and the preset apply * cycle
        // inline. Application installs one so the wait does not hold the
        // command lock.
        commands::TourRunner* tour = nullptr;

        Context(cameras::CameraManager& mgr, cameras::SceneStore& scenes, std::ostream& output = std::cout,
                std::ostream& error = std::cerr)
            : cameraMgr(mgr), sceneStore(scenes), out(output), err(error) {}

        void notifySetupChanged() {
            if (onSetupChanged) {
                onSetupChanged();
            }
        }
    };
} // namespace data

#endif // DATA_CONTEXT_H
