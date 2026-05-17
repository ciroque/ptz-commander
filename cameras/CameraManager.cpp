#include "CameraManager.h"
#include "../core/Logger.h"

namespace cameras {
    std::list<std::shared_ptr<Camera>> CameraManager::getCameras() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cameras_;  // Returns a copy for safety
    }

    std::shared_ptr<Camera> CameraManager::findById(const std::string& id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(cameras_.begin(), cameras_.end(),
            [&id](const auto& cam) {
                return cam->getAlias() == id || cam->getSerialNumber() == id || cam->getName() == id;
            });
        return (it != cameras_.end()) ? *it : nullptr;
    }

    void CameraManager::addCamera(std::shared_ptr<Camera> camera) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!camera) return;  // Guard against null

        auto sn = camera->getSerialNumber();
        auto it = std::find_if(cameras_.begin(), cameras_.end(),
            [&sn](const auto& c) { return c->getSerialNumber() == sn; });
        if (it == cameras_.end()) {
            cameras_.push_back(camera);
            core::Logger::info("CameraManager added camera: " + sn);
        }
    }

    void CameraManager::removeCamera(const std::string& serialNumber) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(cameras_.begin(), cameras_.end(),
            [&serialNumber](const auto& c) { return c->getSerialNumber() == serialNumber; });
        if (it != cameras_.end()) {
            cameras_.erase(it);
            core::Logger::info("CameraManager removed camera: " + serialNumber);
        }
    }
}