#ifndef CAMERAS_VISCA_VISCA_CAMERA_H
#define CAMERAS_VISCA_VISCA_CAMERA_H

#include "../Camera.h"
#include "SerialTransport.h"
#include "ViscaCommands.h"

#include <memory>
#include <string>

namespace cameras::visca {

class ViscaCamera : public ::cameras::Camera {
public:
    // Construct with explicit port (and optional baud/address).
    // The camera will attempt to open the serial port on first use (or eagerly in ctor).
    explicit ViscaCamera(const std::string& port, int baud = 9600, uint8_t address = 1);

    ~ViscaCamera() override;

    // Camera interface
    std::string getSerialNumber() const override;
    std::string getName() const override;
    bool isConnected() const override;

    bool setPosition(float pan, float tilt, int zoom) override;
    bool setZoom(int zoom, int speed) override;
    Ptz getCurrentPtz() const override;

private:
    bool ensureOpen();

    std::unique_ptr<SerialTransport> transport_;
    std::string port_;
    int baud_;
    uint8_t address_;

    Ptz lastPtz_{0.0f, 0.0f, 0};
    bool connected_ = false;
};

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_CAMERA_H
