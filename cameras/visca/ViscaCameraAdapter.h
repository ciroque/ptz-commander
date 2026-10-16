#ifndef CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
#define CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H

#include "../CameraManager.h"
#include "ViscaIpEndpoint.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace cameras::visca {

class ViscaCameraAdapter {
public:
    explicit ViscaCameraAdapter(CameraManager& manager);
    ~ViscaCameraAdapter();

    void start();
    void stop();

    bool addIpCamera(const ViscaIpEndpoint& endpoint);
    const std::vector<ViscaIpEndpoint>& ipEndpoints() const { return ipEndpoints_; }

private:
    void discoverAndAdd();

    CameraManager& manager_;
    std::vector<ViscaIpEndpoint> ipEndpoints_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
