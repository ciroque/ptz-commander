#ifndef CAMERAS_CAMERA_MANAGER_H
#define CAMERAS_CAMERA_MANAGER_H

#include "ICamera.h"
#include <list>
#include <memory>
#include <mutex>

namespace cameras {  // Note: Using 'cameras' namespace per directory structure
    class CameraManager {
    public:
        CameraManager() = default;
        ~CameraManager() = default;

        // Get the current list of cameras (thread-safe)
        std::list<std::shared_ptr<ICamera>> getCameras() const;
        std::shared_ptr<ICamera> getFirstCamera() const;

        // Methods for adapters to update the camera list
        void addCamera(std::shared_ptr<ICamera> camera);
        void removeCamera(const std::string& serialNumber);

    private:
        mutable std::mutex mutex_;  // Protects the camera list
        std::list<std::shared_ptr<ICamera>> cameras_;  // List of cameras
    };
}

#endif // CAMERAS_CAMERA_MANAGER_H