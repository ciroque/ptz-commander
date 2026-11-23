#include "AbsentObsbot.h"

namespace cameras {

    namespace {

        void copySetup(const Camera& from, Camera& to) {
            to.setAlias(from.getAlias());
            to.clearPresets();
            for (const auto& preset : from.getPresets()) {
                to.addPreset(preset.name, preset);
            }
        }

    } // namespace

    AbsentObsbotCamera::AbsentObsbotCamera(std::string serial) : serial_(std::move(serial)) {}

    std::string AbsentObsbotCamera::getSerialNumber() const { return serial_; }

    std::string AbsentObsbotCamera::getName() const { return serial_; }

    bool AbsentObsbotCamera::isConnected() const { return false; }

    ConnectionState AbsentObsbotCamera::connectionState() const { return ConnectionState::Unplugged; }

    bool AbsentObsbotCamera::setPosition(float, float, int) { return false; }

    bool AbsentObsbotCamera::setZoom(int, int) { return false; }

    Ptz AbsentObsbotCamera::getCurrentPtz() const { return {}; }

    bool isObsbotSerial(const std::string& serial) {
        return serial.rfind("VISCA-IP:", 0) != 0 && serial.rfind("VISCA:", 0) != 0 && serial.rfind("ONVIF:", 0) != 0;
    }

    void retainUnpluggedObsbot(CameraManager& mgr, const StagedSetup& staged) {
        for (const auto& [serial, entry] : staged.cameras) {
            if (!isObsbotSerial(serial) || mgr.findBySerial(serial)) {
                continue;
            }
            auto camera = std::make_shared<AbsentObsbotCamera>(serial);
            for (const auto& preset : entry.presets) {
                camera->addPreset(preset.name, preset);
            }
            mgr.addCamera(camera);
            if (entry.alias) {
                mgr.assignAlias(camera, *entry.alias);
            }
        }
    }

    void shelveObsbot(CameraManager& mgr, const std::string& serial) {
        auto live = mgr.findBySerial(serial);
        if (!live || dynamic_cast<AbsentObsbotCamera*>(live.get()) != nullptr) {
            return;
        }
        auto placeholder = std::make_shared<AbsentObsbotCamera>(serial);
        copySetup(*live, *placeholder);
        mgr.removeCamera(serial);
        mgr.addCamera(placeholder);
    }

    void restoreObsbot(CameraManager& mgr, const std::shared_ptr<Camera>& live) {
        if (!live) {
            return;
        }
        auto kept = mgr.findBySerial(live->getSerialNumber());
        if (kept && dynamic_cast<AbsentObsbotCamera*>(kept.get()) != nullptr) {
            copySetup(*kept, *live);
            mgr.removeCamera(live->getSerialNumber());
            mgr.addCamera(live);
            return;
        }
        if (!kept) {
            mgr.addCamera(live);
        }
    }

} // namespace cameras
