#ifndef CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
#define CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H

#include "../CameraManager.h"

#include <atomic>
#include <thread>

namespace cameras::visca {

class ViscaCameraAdapter {
public:
    explicit ViscaCameraAdapter(CameraManager& manager);
    ~ViscaCameraAdapter();

    void start();
    void stop();

private:
    void discoverAndAdd();

    CameraManager& manager_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
