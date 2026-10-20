#ifndef CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
#define CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H

#include "../CameraManager.h"
#include "ViscaCalibration.h"
#include "ViscaIpEndpoint.h"

#include <atomic>
#include <optional>
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

    // Adds the camera only if it answers a VISCA inquiry. False with error
    // set if it can't be added.
    bool addIpCamera(const ViscaIpEndpoint& endpoint, std::string& error);
    // For setup load: adds the camera, or switches an existing one to the
    // endpoint's profile so the file's presets land where they were saved.
    bool loadIpCamera(const ViscaIpEndpoint& endpoint);
    // Calibrates a VISCA-IP camera and switches it to the measured profile.
    std::optional<ViscaProfile> calibrateIpCamera(const std::string& serial,
                                                  const CalibrationRanges& ranges,
                                                  const ViscaCalibrator::Progress& progress,
                                                  std::string& error);
    const std::vector<ViscaIpEndpoint>& ipEndpoints() const { return ipEndpoints_; }

private:
    void discoverAndAdd();
    bool createIpCamera(const ViscaIpEndpoint& endpoint, bool requireReply, std::string& error);

    CameraManager& manager_;
    std::vector<ViscaIpEndpoint> ipEndpoints_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
