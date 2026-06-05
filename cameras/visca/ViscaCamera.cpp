#include "ViscaCamera.h"

#include "WindowsSerialTransport.h"
#include "../../core/Logger.h"

#include <sstream>

namespace cameras::visca {

ViscaCamera::ViscaCamera(const std::string& port, int baud, uint8_t address)
    : port_(port)
    , baud_(baud)
    , address_(address)
{
    // Use Windows implementation (the only one for now)
    transport_ = std::make_unique<WindowsSerialTransport>();

    // Try to open eagerly so isConnected() is truthful early.
    // If it fails we stay disconnected until a command forces a retry.
    connected_ = ensureOpen();

    if (connected_) {
        core::Logger::info("ViscaCamera ready on " + getSerialNumber());
    } else {
        core::Logger::warn("ViscaCamera created but could not open " + port_ + " immediately");
    }
}

ViscaCamera::~ViscaCamera() {
    if (transport_) {
        transport_->close();
    }
}

std::string ViscaCamera::getSerialNumber() const {
    // Synthetic but stable ID so that presets, aliases, findById, etc. continue to work.
    // Example: VISCA:COM3:1
    std::ostringstream oss;
    oss << "VISCA:" << port_ << ":" << static_cast<int>(address_);
    return oss.str();
}

std::string ViscaCamera::getName() const {
    std::ostringstream oss;
    oss << "VISCA Serial (" << port_ << " @" << baud_ << ")";
    return oss.str();
}

bool ViscaCamera::isConnected() const {
    return connected_ && transport_ && transport_->isOpen();
}

bool ViscaCamera::ensureOpen() {
    if (transport_ && transport_->isOpen()) {
        return true;
    }
    if (transport_) {
        connected_ = transport_->open(port_, baud_);
        return connected_;
    }
    return false;
}

bool ViscaCamera::setPosition(float pan, float tilt, int zoom) {
    if (!ensureOpen() || !transport_) {
        core::Logger::error("ViscaCamera::setPosition - transport not open on " + port_);
        return false;
    }

    auto ptCmd = ViscaCommands::panTiltAbsolute(pan, tilt, 0x10, 0x10, address_);
    auto zmCmd = ViscaCommands::zoomAbsolute(zoom, address_);

    bool ok = transport_->write(ptCmd) && transport_->write(zmCmd);

    if (ok) {
        lastPtz_.pan  = pan;
        lastPtz_.tilt = tilt;
        lastPtz_.zoom = zoom;
        core::Logger::info("VISCA sent absolute PTZ to " + getSerialNumber() +
                           " pan=" + std::to_string(pan) +
                           " tilt=" + std::to_string(tilt) +
                           " zoom=" + std::to_string(zoom));
    } else {
        core::Logger::error("VISCA failed to send absolute PTZ on " + port_);
    }

    return ok;
}

bool ViscaCamera::setZoom(int zoom, int /*speed*/) {
    // Speed is accepted for interface compatibility but ignored for absolute direct zoom.
    // (VISCA direct zoom command does not take a speed parameter.)
    if (!ensureOpen() || !transport_) {
        core::Logger::error("ViscaCamera::setZoom - transport not open on " + port_);
        return false;
    }

    auto cmd = ViscaCommands::zoomAbsolute(zoom, address_);
    bool ok = transport_->write(cmd);

    if (ok) {
        lastPtz_.zoom = zoom;
        core::Logger::info("VISCA sent absolute zoom " + std::to_string(zoom) + " to " + getSerialNumber());
    } else {
        core::Logger::error("VISCA zoom command failed on " + port_);
    }

    return ok;
}

Ptz ViscaCamera::getCurrentPtz() const {
    // Return the last successfully commanded position (optimistic cache).
    // This is consistent with how some OBSBOT paths report state when fresh telemetry
    // isn't continuously available. Real inquiry support can be added later.
    return lastPtz_;
}

} // namespace cameras::visca
