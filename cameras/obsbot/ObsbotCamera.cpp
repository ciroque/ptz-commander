#include "ObsbotCamera.h"

// Full includes - this is where the compiler finally sees the real class
#include "strategies/ObsbotControlStrategy.h"
#include "strategies/TinyFamilyStrategy.h"
#include "strategies/TailAirStrategy.h"

#include <iostream>

namespace cameras {

    namespace {
        std::unique_ptr<cameras::obsbot::strategies::ObsbotControlStrategy> createStrategy(ObsbotProductType type) {
            switch (type) {
            case ObsbotProdTailAir:
            case ObsbotProdTail2:
            case ObsbotProdTail2S:
                return std::make_unique<cameras::obsbot::strategies::TailAirStrategy>();
            default:
                return std::make_unique<cameras::obsbot::strategies::TinyFamilyStrategy>();
            }
        }
    }

    ObsbotCamera::ObsbotCamera(std::shared_ptr<Device> dev)
        : device_(std::move(dev))
    {
        if (!device_) return;

        strategy_ = createStrategy(device_->productType());

        std::string sn = getSerialNumber();
        std::cout << "Initializing camera: " << getName() << " (" << sn << ")" << std::endl;

        if (strategy_ && strategy_->disableAI(device_.get())) {
            std::cout << "✅ AI / Tracking fully disabled for " << sn << std::endl;
        }
        else {
            std::cout << "Warning: Failed to fully disable AI for " << sn << std::endl;
        }
    }

    ObsbotCamera::~ObsbotCamera() = default;   // crucial

    std::string ObsbotCamera::getSerialNumber() const {
        return device_ ? device_->devSn() : "Unknown";
    }

    std::string ObsbotCamera::getName() const {
        return device_ ? device_->devName() : "Unknown";
    }

    bool ObsbotCamera::isConnected() const {
        return device_ != nullptr;
    }

    bool ObsbotCamera::setPosition(float pan, float tilt, int zoom) {
        return strategy_ ? strategy_->moveTo(pan, tilt, zoom, device_.get()) : false;
    }

    bool ObsbotCamera::setZoom(int zoom, int speed) {
        return strategy_ ? strategy_->setZoom(zoom, speed, device_.get()) : false;
    }

    Ptz ObsbotCamera::getCurrentPtz() const {
        Ptz ptz{ 0.0f, 0.0f, 0 };
        if (!device_) return ptz;

        float pos[3] = { 0 };
        if (device_->gimbalGetAttitudeInfoR(pos) == RM_RET_OK) {
            ptz.tilt = pos[1];
            ptz.pan = pos[2];
        }

        float zoomVal = 0.0f;
        if (device_->cameraGetZoomAbsoluteR(zoomVal) == RM_RET_OK) {
            ptz.zoom = static_cast<int>((zoomVal - 1.0f) * 100.0f);
        }
        return ptz;
    }

} // namespace cameras