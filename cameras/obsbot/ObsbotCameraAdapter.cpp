#include "ObsbotCameraAdapter.h"
#include "ObsbotCamera.h"
#include "../../include/obsbot/comm.hpp"

#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <array>

namespace cameras::obsbot {

    // Thread-safe SDK log handler → file
    namespace {
        std::ofstream logFile;
        std::mutex logMutex;

        void sdkLogHandler(int32_t lvl, const char* msg, va_list args, void* /*param*/) {
            std::lock_guard<std::mutex> lock(logMutex);
            if (!logFile.is_open()) return;

            // Timestamp
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm timeBuf{};
            localtime_s(&timeBuf, &time_t);

            char timeStr[64];
            std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeBuf);

            const char* levelStr = "UNKNOWN";
            if (lvl == DEV_ERROR) levelStr = "ERROR";
            else if (lvl == DEV_WARN)  levelStr = "WARN";
            else if (lvl == DEV_INFO)  levelStr = "INFO";
            else if (lvl == DEV_DEBUG) levelStr = "DEBUG";

            // Format message into buffer
            std::array<char, 1024> buffer{};
            vsnprintf(buffer.data(), buffer.size(), msg, args);

            logFile << "[" << timeStr << "] [" << levelStr << "] "
                << buffer.data() << std::endl;
            logFile.flush();
        }
    }

    ObsbotCameraAdapter::ObsbotCameraAdapter(CameraManager& manager)
        : devices_(Devices::get()), manager_(manager), running_(false) {

        // === SDK Logging Redirect ===
        logFile.open("obsbot_sdk.log", std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            std::cout << "✅ OBSBOT SDK logging redirected to: obsbot_sdk.log" << std::endl;
            dev_set_log_handler(sdkLogHandler, nullptr);
        }
        else {
            std::cerr << "❌ Failed to open obsbot_sdk.log for writing" << std::endl;
        }

        devices_.setEnableMdnsScan(true);
    }

    ObsbotCameraAdapter::~ObsbotCameraAdapter() {
        stop();
        if (logFile.is_open()) {
            logFile.close();
        }
        // Optional: restore default handler
        // dev_set_log_handler(nullptr, nullptr);
    }

    void ObsbotCameraAdapter::start() {
        if (!running_) {
            running_ = true;

            devices_.startNetworkScanImmediately();

            findDevices();

            devices_.setDevChangedCallback(onDeviceChanged, this);

            std::cout << "ObsbotCameraAdapter started (USB + Network + SDK logging to file)." << std::endl;
        }
    }

    void ObsbotCameraAdapter::stop() {
        if (running_) {
            running_ = false;
            devices_.setDevChangedCallback(nullptr, nullptr);
            std::cout << "ObsbotCameraAdapter stopped." << std::endl;
        }
    }

    void ObsbotCameraAdapter::onDeviceChanged(std::string devSn, bool connected, void* param) {
        auto* adapter = static_cast<ObsbotCameraAdapter*>(param);
        if (!adapter) return;

        std::cout << "Device event: " << devSn << " → " << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;

        if (connected) {
            auto dev = adapter->devices_.getDevBySn(devSn);
            if (dev) {
                if (dev->devMode() == Device::DevModeUvc ||
                    dev->devMode() == Device::DevModeNet) {

                    std::string mode = (dev->devMode() == Device::DevModeNet) ? "NETWORK" : "USB";
                    std::cout << "Adding " << mode << " device: "
                        << dev->devName() << " (" << devSn << ")" << std::endl;

                    adapter->manager_.addCamera(std::make_shared<ObsbotCamera>(dev));
                }
            }
        }
        else {
            adapter->manager_.removeCamera(devSn);
        }
    }

    void ObsbotCameraAdapter::findDevices() {
        auto devList = devices_.getDevList();
        for (const auto& dev : devList) {
            if (dev && (dev->devMode() == Device::DevModeUvc ||
                dev->devMode() == Device::DevModeNet)) {
                std::cout << "Pre-existing device found: " << dev->devName() << std::endl;
                manager_.addCamera(std::make_shared<ObsbotCamera>(dev));
            }
        }
    }

} // namespace cameras::obsbot