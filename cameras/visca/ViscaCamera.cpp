#include "ViscaCamera.h"

#include "WindowsSerialTransport.h"
#include "../../core/Logger.h"

#include <memory>

namespace cameras::visca {

ViscaCamera::ViscaCamera(const std::string& port, int baud, uint8_t address)
    : ViscaCamera(std::make_unique<WindowsSerialTransport>(port, baud),
                  "VISCA:" + port + ":" + std::to_string(static_cast<int>(address)),
                  "VISCA Serial (" + port + " @" + std::to_string(baud) + ")",
                  address,
                  lumensVcA51p()) {}

ViscaCamera::ViscaCamera(std::unique_ptr<ViscaTransport> transport,
                         std::string serial,
                         std::string name,
                         uint8_t address,
                         ViscaProfile profile)
    : transport_(std::move(transport))
    , serial_(std::move(serial))
    , name_(std::move(name))
    , address_(address)
    , profile_(profile)
{
    connected_ = ensureOpen();
    if (connected_) {
        core::Logger::info("ViscaCamera ready on " + serial_);
    } else {
        core::Logger::warn("ViscaCamera created but could not open " + serial_ + " immediately");
    }
}

ViscaCamera::~ViscaCamera() {
    if (transport_) {
        transport_->close();
    }
}

std::string ViscaCamera::getSerialNumber() const {
    return serial_;
}

std::string ViscaCamera::getName() const {
    return name_;
}

bool ViscaCamera::isConnected() const {
    return connected_ && transport_ && transport_->isOpen();
}

bool ViscaCamera::answersInquiry() const {
    if (!ensureOpen() || !transport_) {
        return false;
    }
    // Asked twice, in case one datagram is lost.
    for (int attempt = 0; attempt < 2; ++attempt) {
        transport_->purgeInput();
        if (!transport_->write(ViscaCommands::panTiltPositionInquiry(address_))) {
            return false;
        }
        // A reply from camera address 1-7 starts with 0x90-0xF0 and ends with FF.
        const auto reply = transport_->readPacket(1000);
        if (reply.size() >= 3 && reply[0] >= 0x90 && (reply[0] & 0x0F) == 0 && reply.back() == 0xFF) {
            return true;
        }
    }
    return false;
}

bool ViscaCamera::ensureOpen() const {
    if (transport_ && transport_->isOpen()) {
        return true;
    }
    if (transport_) {
        connected_ = transport_->open();
        return connected_;
    }
    return false;
}

bool ViscaCamera::setPosition(float pan, float tilt, int zoom) {
    if (!ensureOpen() || !transport_) {
        core::Logger::error("ViscaCamera::setPosition - transport not open on " + serial_);
        return false;
    }

    // Compute the actual values that will be sent (ViscaCommands clamps to camera limits)
    float clampedPan  = ViscaCommands::viscaToPanDegrees(
        ViscaCommands::panDegreesToVisca(pan, profile_), profile_);
    float clampedTilt = ViscaCommands::viscaToTiltDegrees(
        ViscaCommands::tiltDegreesToVisca(tilt, profile_), profile_);
    int   clampedZoom = ViscaCommands::viscaToZoomPercent(
        ViscaCommands::zoomPercentToVisca(zoom, profile_), profile_);

    auto ptCmd = ViscaCommands::panTiltAbsolute(pan, tilt, profile_, 0x10, 0x10, address_);
    auto zmCmd = ViscaCommands::zoomAbsolute(zoom, profile_, address_);

    bool ok = transport_->write(ptCmd) && transport_->write(zmCmd);

    if (ok) {
        lastPtz_.pan  = clampedPan;
        lastPtz_.tilt = clampedTilt;
        lastPtz_.zoom = clampedZoom;
        core::Logger::info("VISCA sent absolute PTZ to " + getSerialNumber() +
                           " pan=" + std::to_string(clampedPan) +
                           " tilt=" + std::to_string(clampedTilt) +
                           " zoom=" + std::to_string(clampedZoom));
    } else {
        core::Logger::error("VISCA failed to send absolute PTZ on " + serial_);
    }

    return ok;
}

bool ViscaCamera::setZoom(int zoom, int /*speed*/) {
    // Speed is accepted for interface compatibility but ignored for absolute direct zoom.
    // (VISCA direct zoom command does not take a speed parameter.)
    if (!ensureOpen() || !transport_) {
        core::Logger::error("ViscaCamera::setZoom - transport not open on " + serial_);
        return false;
    }

    int clampedZoom = ViscaCommands::viscaToZoomPercent(
        ViscaCommands::zoomPercentToVisca(zoom, profile_), profile_);
    auto cmd = ViscaCommands::zoomAbsolute(zoom, profile_, address_);
    bool ok = transport_->write(cmd);

    if (ok) {
        lastPtz_.zoom = clampedZoom;  // use clamped value because ViscaCommands clamps caller-provided zoom to [0,100]
        core::Logger::info("VISCA sent absolute zoom " + std::to_string(clampedZoom) + " to " + getSerialNumber());
    } else {
        core::Logger::error("VISCA zoom command failed on " + serial_);
    }

    return ok;
}

std::optional<ViscaProfile> ViscaCamera::calibrate(const CalibrationRanges& ranges,
                                                   const ViscaCalibrator::Progress& progress,
                                                   std::string& error) {
    if (!ensureOpen() || !transport_) {
        error = "transport not open on " + serial_;
        return std::nullopt;
    }
    ViscaCalibrator calibrator(*transport_, address_, progress);
    return calibrator.run(ranges, error);
}

Ptz remapPtz(const Ptz& ptz, const ViscaProfile& from, const ViscaProfile& to) {
    return Ptz{
        ViscaCommands::viscaToPanDegrees(ViscaCommands::panDegreesToVisca(ptz.pan, from), to),
        ViscaCommands::viscaToTiltDegrees(ViscaCommands::tiltDegreesToVisca(ptz.tilt, from), to),
        ViscaCommands::viscaToZoomPercent(ViscaCommands::zoomPercentToVisca(ptz.zoom, from), to)};
}

void ViscaCamera::applyProfile(const ViscaProfile& profile) {
    for (const auto& preset : getPresets()) {
        AddPreset(preset->name, Preset(preset->name, remapPtz(preset->ptz, profile_, profile)));
    }
    lastPtz_ = remapPtz(lastPtz_, profile_, profile);
    profile_ = profile;
    core::Logger::info("ViscaCamera " + serial_ + " now using profile " + profile_.name);
}

Ptz ViscaCamera::getCurrentPtz() const {
    if (!ensureOpen() || !transport_) {
        core::Logger::warn("ViscaCamera::getCurrentPtz - transport not open on " + serial_ + ", returning cached");
        return lastPtz_;
    }

    // Purge any stale data before inquiries
    transport_->purgeInput();

    // --- Pan/Tilt Position Inquiry ---
    auto ptInq = ViscaCommands::panTiltPositionInquiry(address_);
    if (!transport_->write(ptInq)) {
        core::Logger::error("VISCA pan/tilt inquiry write failed on " + serial_);
        return lastPtz_;
    }
    auto ptResp = transport_->readPacket(800);  // generous timeout for response

    uint16_t panVisca = 0, tiltVisca = 0;
    bool ptOk = false;
    if (const auto pt = ViscaCommands::parsePanTiltPosition(ptResp)) {
        panVisca = pt->pan;
        tiltVisca = pt->tilt;
        ptOk = true;
    } else if (!ptResp.empty()) {
        core::Logger::debug("VISCA pan/tilt inquiry bad response on " + serial_ +
                            " (len=" + std::to_string(ptResp.size()) + ")");
    }

    // --- Zoom Position Inquiry ---
    transport_->purgeInput();  // fresh for next inquiry

    auto zmInq = ViscaCommands::zoomPositionInquiry(address_);
    if (!transport_->write(zmInq)) {
        core::Logger::error("VISCA zoom inquiry write failed on " + serial_);
        // still return what we have for pan/tilt if successful
        if (ptOk) {
            lastPtz_.pan = ViscaCommands::viscaToPanDegrees(panVisca, profile_);
            lastPtz_.tilt = ViscaCommands::viscaToTiltDegrees(tiltVisca, profile_);
        }
        return lastPtz_;
    }
    auto zmResp = transport_->readPacket(500);

    uint16_t zoomVisca = 0;
    bool zmOk = false;
    if (const auto zm = ViscaCommands::parseZoomPosition(zmResp)) {
        zoomVisca = *zm;
        zmOk = true;
    } else if (!zmResp.empty()) {
        core::Logger::debug("VISCA zoom inquiry bad response on " + serial_ +
                            " (len=" + std::to_string(zmResp.size()) + ")");
    }

    // Update cache with whatever we successfully read
    if (ptOk) {
        lastPtz_.pan = ViscaCommands::viscaToPanDegrees(panVisca, profile_);
        lastPtz_.tilt = ViscaCommands::viscaToTiltDegrees(tiltVisca, profile_);
    }
    if (zmOk) {
        lastPtz_.zoom = ViscaCommands::viscaToZoomPercent(zoomVisca, profile_);
    }

    if (!ptOk && !zmOk) {
        core::Logger::warn("VISCA getCurrentPtz inquiries failed on " + serial_ + ", returning last cached");
    } else {
        core::Logger::debug("VISCA read live PTZ on " + getSerialNumber() +
                            " pan=" + std::to_string(lastPtz_.pan) +
                            " tilt=" + std::to_string(lastPtz_.tilt) +
                            " zoom=" + std::to_string(lastPtz_.zoom));
    }

    return lastPtz_;
}

} // namespace cameras::visca
