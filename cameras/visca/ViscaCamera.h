#ifndef CAMERAS_VISCA_VISCA_CAMERA_H
#define CAMERAS_VISCA_VISCA_CAMERA_H

#include "../Camera.h"
#include "ViscaProfile.h"
#include "ViscaTransport.h"
#include "ViscaCommands.h"

#include <memory>
#include <string>

namespace cameras::visca {

class ViscaCamera : public ::cameras::Camera {
public:
    explicit ViscaCamera(const std::string& port, int baud = 9600, uint8_t address = 1);

    ViscaCamera(std::unique_ptr<ViscaTransport> transport,
                std::string serial,
                std::string name,
                uint8_t address,
                ViscaProfile profile);

    ~ViscaCamera() override;

    std::string getSerialNumber() const override;
    std::string getName() const override;
    bool isConnected() const override;

    bool setPosition(float pan, float tilt, int zoom) override;
    bool setZoom(int zoom, int speed) override;
    Ptz getCurrentPtz() const override;

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
