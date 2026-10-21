#ifndef CAMERAS_VISCA_VISCA_CAMERA_H
#define CAMERAS_VISCA_VISCA_CAMERA_H

#include "../Camera.h"
#include "ViscaCalibration.h"
#include "ViscaProfile.h"
#include "ViscaTransport.h"
#include "ViscaCommands.h"

#include <memory>
#include <optional>
#include <string>

namespace cameras::visca {

// The degrees that point at the same raw position under `to` as ptz does
// under `from`.
Ptz remapPtz(const Ptz& ptz, const ViscaProfile& from, const ViscaProfile& to);

class ViscaCamera : public ::cameras::Camera {
public:
    ViscaCamera(std::unique_ptr<ViscaTransport> transport,
                std::string serial,
                std::string name,
                uint8_t address,
                ViscaProfile profile);

    ~ViscaCamera() override;

    std::string getSerialNumber() const override;
    std::string getName() const override;
    // Whether the camera answers a position inquiry now, not just whether its
    // serial port or socket is open. Up to 0.6 s for one that doesn't answer.
    bool isConnected() const override;
    // Whether the serial port or UDP socket is open (opening it if needed).
    bool isOpen() const;

    bool setPosition(float pan, float tilt, int zoom) override;
    bool setZoom(int zoom, int speed) override;
    Ptz getCurrentPtz() const override;

    // Sends a pan/tilt position inquiry, up to twice with timeoutMs each, and
    // reports whether a VISCA reply came back. An error reply counts: it
    // proves a VISCA device is listening.
    bool answersInquiry(int timeoutMs = 1000) const;

    const ViscaProfile& profile() const { return profile_; }

    // Drives the camera through its full range to measure a profile. Does not
    // apply it; see applyProfile.
    std::optional<ViscaProfile> calibrate(const CalibrationRanges& ranges,
                                          const ViscaCalibrator::Progress& progress,
                                          std::string& error);

    // Switches profiles. Presets are stored in degrees, so each one is
    // converted through its raw position to keep pointing at the same spot.
    void applyProfile(const ViscaProfile& profile);

private:
    bool ensureOpen() const;

    std::unique_ptr<ViscaTransport> transport_;
    std::string serial_;
    std::string name_;
    uint8_t address_ = 1;
    ViscaProfile profile_{lumensVcA51p()};

    mutable Ptz lastPtz_{0.0f, 0.0f, 0};
    mutable bool connected_ = false;
};

} // namespace cameras::visca

#endif
