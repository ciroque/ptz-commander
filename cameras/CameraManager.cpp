#include "CameraManager.h"
#include <iostream>

namespace cameras {
    std::list<std::shared_ptr<ICamera>> CameraManager::getCameras() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cameras_;  // Returns a copy for safety
    }

    std::shared_ptr<ICamera> CameraManager::findBySerialNumber(const std::string& sn) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(cameras_.begin(), cameras_.end(),
            [&sn](const auto& cam) { return cam->getSerialNumber() == sn; });
        return (it != cameras_.end()) ? *it : nullptr;
    }

    void CameraManager::addCamera(std::shared_ptr<ICamera> camera) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!camera) return;  // Guard against null

        auto sn = camera->getSerialNumber();
        auto it = std::find_if(cameras_.begin(), cameras_.end(),
            [&sn](const auto& c) { return c->getSerialNumber() == sn; });
        if (it == cameras_.end()) {
            cameras_.push_back(camera);
            std::cout << "CameraManager added camera: " << sn << std::endl;
        }
    }

    void CameraManager::removeCamera(const std::string& serialNumber) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(cameras_.begin(), cameras_.end(),
            [&serialNumber](const auto& c) { return c->getSerialNumber() == serialNumber; });
        if (it != cameras_.end()) {
            cameras_.erase(it);
            std::cout << "CameraManager removed camera: " << serialNumber << std::endl;
        }
    }
}