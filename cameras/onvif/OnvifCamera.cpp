#include "OnvifCamera.h"

#include "OnvifMap.h"
#include "../../core/Logger.h"

namespace cameras::onvif {

    OnvifCamera::OnvifCamera(OnvifEndpoint endpoint)
        : endpoint_(std::move(endpoint)), serial_(endpoint_.serial()), name_("ONVIF (" + endpoint_.address() + ")"),
          client_(endpoint_.host, endpoint_.port, endpoint_.servicePath, endpoint_.user, endpoint_.password) {
        if (endpoint_.port <= 0) {
            endpoint_.port = 80;
            serial_ = endpoint_.serial();
        }
    }

    ::cameras::ConnectionState OnvifCamera::connectionState() const {
        if (connected_) {
            return ::cameras::ConnectionState::Connected;
        }
        // A stored password the camera rejected still needs a login.
        // Any other failure, including no answer, stays offline.
        if (endpoint_.password.empty() || lastKind_ == OnvifError::AuthRejected) {
            return ::cameras::ConnectionState::NeedsLogin;
        }
        return ::cameras::ConnectionState::Offline;
    }

    void OnvifCamera::noteAuthRejected() {
        lastKind_ = OnvifError::AuthRejected;
        connected_ = false;
    }

    bool OnvifCamera::probe(std::string& error) {
        OnvifDeviceInfo info;
        const auto prepared = client_.prepare(info);
        if (!prepared.ok) {
            error = prepared.detail;
            lastKind_ = prepared.kind;
            connected_ = false;
            return false;
        }
        lastKind_ = OnvifError::None;
        if (!info.manufacturer.empty() && !info.model.empty()) {
            name_ = info.manufacturer + " " + info.model;
        } else if (!info.model.empty()) {
            name_ = info.model;
        } else if (!info.manufacturer.empty()) {
            name_ = info.manufacturer;
        }
        connected_ = true;
        return true;
    }

    bool OnvifCamera::signIn(const std::string& password, std::string& error) {
        const std::string previous = endpoint_.password;
        const bool wasConnected = connected_;
        const bool hadRaw = haveRaw_;
        const OnvifPosition raw = lastRaw_;
        const Ptz ptz = lastPtz_;
        endpoint_.password = password;
        client_ = OnvifClient(endpoint_.host, endpoint_.port, endpoint_.servicePath, endpoint_.user, password);
        haveRaw_ = false;
        if (!probe(error)) {
            endpoint_.password = previous;
            client_ = OnvifClient(endpoint_.host, endpoint_.port, endpoint_.servicePath, endpoint_.user, previous);
            connected_ = wasConnected;
            haveRaw_ = hadRaw;
            lastRaw_ = raw;
            lastPtz_ = ptz;
            return false;
        }
        return true;
    }

    std::string OnvifCamera::getSerialNumber() const { return serial_; }

    std::string OnvifCamera::getName() const { return name_; }

    bool OnvifCamera::isConnected() const { return connected_; }

    bool OnvifCamera::moveRaw(double panX, double tiltY, double zoomX, double panTiltSpeed, double zoomSpeed) const {
        const auto moved = client_.absoluteMove(panX, tiltY, zoomX, panTiltSpeed, zoomSpeed);
        if (!moved.ok) {
            core::Logger::error("ONVIF move failed on " + serial_ + ": " + moved.detail);
            return false;
        }
        lastRaw_.panX = panX;
        lastRaw_.tiltY = tiltY;
        lastRaw_.zoomX = zoomX;
        haveRaw_ = true;
        lastPtz_.pan = xToPanDegrees(panX);
        lastPtz_.tilt = yToTiltDegrees(tiltY);
        lastPtz_.zoom = xToZoomPercent(zoomX);
        return true;
    }

    bool OnvifCamera::setPosition(float pan, float tilt, int zoom) {
        if (!connected_) {
            core::Logger::error("ONVIF setPosition on an inactive camera: " + serial_);
            return false;
        }
        const double panX = panDegreesToX(pan);
        const double tiltY = tiltDegreesToY(tilt);
        const double zoomX = zoomPercentToX(zoom);
        if (!moveRaw(panX, tiltY, zoomX, 0.8, 0.8)) {
            return false;
        }
        core::Logger::info("ONVIF absolute move " + serial_ + " pan=" + std::to_string(lastPtz_.pan)
                           + " tilt=" + std::to_string(lastPtz_.tilt) + " zoom=" + std::to_string(lastPtz_.zoom));
        return true;
    }

    bool OnvifCamera::setZoom(int zoom, int speed) {
        if (!connected_) {
            core::Logger::error("ONVIF setZoom on an inactive camera: " + serial_);
            return false;
        }
        if (!haveRaw_) {
            getCurrentPtz();
        }
        if (!haveRaw_) {
            return false;
        }
        double zoomSpeed = 0.5;
        if (speed > 0) {
            zoomSpeed = speed > 100 ? 1.0 : static_cast<double>(speed) / 100.0;
        }
        return moveRaw(lastRaw_.panX, lastRaw_.tiltY, zoomPercentToX(zoom), 0.8, zoomSpeed);
    }

    Ptz OnvifCamera::getCurrentPtz() const {
        if (!connected_) {
            return lastPtz_;
        }
        OnvifPosition raw;
        const auto status = client_.getStatus(raw);
        if (!status.ok) {
            core::Logger::warn("ONVIF position read failed on " + serial_ + ": " + status.detail);
            return lastPtz_;
        }
        lastRaw_ = raw;
        haveRaw_ = true;
        lastPtz_.pan = xToPanDegrees(raw.panX);
        lastPtz_.tilt = yToTiltDegrees(raw.tiltY);
        lastPtz_.zoom = xToZoomPercent(raw.zoomX);
        return lastPtz_;
    }

} // namespace cameras::onvif
