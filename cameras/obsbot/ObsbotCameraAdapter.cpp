#include "ObsbotCameraAdapter.h"
#include "ObsbotCamera.h"
#include <iostream>
#include <chrono>

namespace cameras::obsbot {
    ObsbotCameraAdapter::ObsbotCameraAdapter(CameraManager& manager)
        : devices_(Devices::get()), manager_(manager), running_(false) {
        devices_.setEnableMdnsScan(false);  // USB only
    }

    ObsbotCameraAdapter::~ObsbotCameraAdapter() {
        stop();
    }

    void ObsbotCameraAdapter::start() {
        if (!running_) {
            running_ = true;

            ObsbotCameraAdapter::findDevices();

            devices_.setDevChangedCallback(onDeviceChanged, this);

            std::cout << "ObsbotCameraAdapter started." << std::endl;

            while (running_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void ObsbotCameraAdapter::stop() {
        if (running_) {
            running_ = false;
            if (watchThread_.joinable()) {
                watchThread_.join();
            }
            devices_.setDevChangedCallback(nullptr, nullptr);
            std::cout << "ObsbotCameraAdapter stopped." << std::endl;
        }
    }

    void ObsbotCameraAdapter::onDeviceChanged(std::string devSn, bool connected, void* param) {
		std::cout << "Device changed: " << devSn << " connected: " << connected << std::endl;
        auto* adapter = static_cast<ObsbotCameraAdapter*>(param);
        if (connected) {
			std::cout << "Device connected: " << devSn << std::endl;
            auto dev = adapter->devices_.getDevBySn(devSn);
            if (dev && dev->devMode() == Device::DevModeUvc) {
                adapter->manager_.addCamera(std::make_shared<ObsbotCamera>(dev));
            }
        }
        else {
			std::cout << "Device disconnected: " << devSn << std::endl;
            adapter->manager_.removeCamera(devSn);
        }
    }

    void ObsbotCameraAdapter::findDevices() {
        auto numDevs = devices_.getDevNum();
        if (numDevs > 0) {
            auto devList = devices_.getDevList();
            for (const auto& dev : devList) {
                std::cout << "Found device: " << dev->devName() << std::endl;
                if (dev && dev->devMode() == Device::DevModeUvc) {
                    manager_.addCamera(std::make_shared<ObsbotCamera>(dev));
                }
            }
        }
    }
}