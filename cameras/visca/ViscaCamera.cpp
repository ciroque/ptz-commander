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

bool ViscaCamera::ensureOpen() const {
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
    if (!ensureOpen() || !transport_) {
        core::Logger::warn("ViscaCamera::getCurrentPtz - transport not open on " + port_ + ", returning cached");
        return lastPtz_;
    }

    // Purge any stale data before inquiries
    transport_->purgeInput();

    // --- Pan/Tilt Position Inquiry ---
    auto ptInq = ViscaCommands::panTiltPositionInquiry(address_);
    if (!transport_->write(ptInq)) {
        core::Logger::error("VISCA pan/tilt inquiry write failed on " + port_);
        return lastPtz_;
    }
    auto ptResp = transport_->readPacket(800);  // generous timeout for response

    uint16_t panVisca = 0, tiltVisca = 0;
    bool ptOk = false;
    if (!ptResp.empty() && ptResp.back() == 0xFF && ptResp.size() >= 11 &&
        ptResp[0] == 0x90 && ptResp[1] == 0x50) {
        panVisca = (static_cast<uint16_t>(ptResp[2]) << 12) |
                   (static_cast<uint16_t>(ptResp[3]) << 8) |
                   (static_cast<uint16_t>(ptResp[4]) << 4) |
                   ptResp[5];
        tiltVisca = (static_cast<uint16_t>(ptResp[6]) << 12) |
                    (static_cast<uint16_t>(ptResp[7]) << 8) |
                    (static_cast<uint16_t>(ptResp[8]) << 4) |
                    ptResp[9];
        ptOk = true;
    } else if (!ptResp.empty()) {
        core::Logger::debug("VISCA pan/tilt inquiry bad response on " + port_ +
                            " (len=" + std::to_string(ptResp.size()) + ")");
    }

    // --- Zoom Position Inquiry ---
    transport_->purgeInput();  // fresh for next inquiry

    auto zmInq = ViscaCommands::zoomPositionInquiry(address_);
    if (!transport_->write(zmInq)) {
        core::Logger::error("VISCA zoom inquiry write failed on " + port_);
        // still return what we have for pan/tilt if successful
        if (ptOk) {
            lastPtz_.pan = ViscaCommands::viscaToPanDegrees(panVisca);
            lastPtz_.tilt = ViscaCommands::viscaToTiltDegrees(tiltVisca);
        }
        return lastPtz_;
    }
    auto zmResp = transport_->readPacket(500);

    uint16_t zoomVisca = 0;
    bool zmOk = false;
    if (!zmResp.empty() && zmResp.back() == 0xFF && zmResp.size() >= 7 &&
        zmResp[0] == 0x90 && zmResp[1] == 0x50) {
        zoomVisca = (static_cast<uint16_t>(zmResp[2]) << 12) |
                    (static_cast<uint16_t>(zmResp[3]) << 8) |
                    (static_cast<uint16_t>(zmResp[4]) << 4) |
                    zmResp[5];
        zmOk = true;
    } else if (!zmResp.empty()) {
        core::Logger::debug("VISCA zoom inquiry bad response on " + port_ +
                            " (len=" + std::to_string(zmResp.size()) + ")");
    }

    // Update cache with whatever we successfully read
    if (ptOk) {
        lastPtz_.pan = ViscaCommands::viscaToPanDegrees(panVisca);
        lastPtz_.tilt = ViscaCommands::viscaToTiltDegrees(tiltVisca);
    }
    if (zmOk) {
        lastPtz_.zoom = ViscaCommands::viscaToZoomPercent(zoomVisca);
    }

    if (!ptOk && !zmOk) {
        core::Logger::warn("VISCA getCurrentPtz inquiries failed on " + port_ + ", returning last cached");
    } else {
        core::Logger::debug("VISCA read live PTZ on " + getSerialNumber() +
                            " pan=" + std::to_string(lastPtz_.pan) +
                            " tilt=" + std::to_string(lastPtz_.tilt) +
                            " zoom=" + std::to_string(lastPtz_.zoom));
    }

    return lastPtz_;
}

} // namespace cameras::visca
