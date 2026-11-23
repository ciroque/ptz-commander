#ifndef CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
#define CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H

#include "../CameraManager.h"
#include "ViscaCalibration.h"
#include "ViscaIpEndpoint.h"
#include "ViscaSerialEndpoint.h"

#include <optional>
#include <string>
#include <vector>

namespace cameras::visca {

    class ViscaCameraAdapter {
    public:
        explicit ViscaCameraAdapter(CameraManager& manager) : manager_(manager) {}

        // Adds the camera only if it answers a VISCA inquiry. False with error
        // set if it can't be added.
        bool addIpCamera(const ViscaIpEndpoint& endpoint, std::string& error);
        // For setup load: adds the camera, or switches an existing one to the
        // endpoint's profile so the file's presets land where they were saved.
        bool loadIpCamera(const ViscaIpEndpoint& endpoint);
        const std::vector<ViscaIpEndpoint>& ipEndpoints() const { return ipEndpoints_; }

        // The same for cameras on COM ports, one per port.
        bool addSerialCamera(const ViscaSerialEndpoint& endpoint, std::string& error);
        bool loadSerialCamera(const ViscaSerialEndpoint& endpoint);
        const std::vector<ViscaSerialEndpoint>& serialEndpoints() const { return serialEndpoints_; }

        // Records an endpoint for a camera already in the manager. Does not
        // open a socket. setup save writes the record.
        bool keepIp(const ViscaIpEndpoint& endpoint);
        bool keepSerial(const ViscaSerialEndpoint& endpoint);

        // Drops the IP or serial record and the camera. False when this
        // adapter has no record for the serial.
        bool discard(const std::string& serial);

        // Calibrates a VISCA camera, IP or serial, switches it to the measured
        // profile and keeps the profile with its endpoint for `setup save`.
        std::optional<ViscaProfile> calibrateCamera(const std::string& serial, const CalibrationRanges& ranges,
                                                    const ViscaCalibrator::Progress& progress, std::string& error);

    private:
        bool createIpCamera(const ViscaIpEndpoint& endpoint, bool requireReply, std::string& error);
        bool createSerialCamera(const ViscaSerialEndpoint& endpoint, bool requireReply, std::string& error);

        CameraManager& manager_;
        std::vector<ViscaIpEndpoint> ipEndpoints_;
        std::vector<ViscaSerialEndpoint> serialEndpoints_;
    };

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_CAMERA_ADAPTER_H
