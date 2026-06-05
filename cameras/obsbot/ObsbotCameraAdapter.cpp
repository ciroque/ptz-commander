#include "ObsbotCameraAdapter.h"
#include "ObsbotCamera.h"
#include "../../include/obsbot/comm.hpp"
#include "../../core/Application.h"
#include "../../core/Logger.h"

#include <fstream>
#include <chrono>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <array>
#include <filesystem>
#include <cstdlib>

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
            auto nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm timeBuf{};
            localtime_s(&timeBuf, &nowTime);

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
        // Write SDK logs to a user-writable location (not Program Files).
        // This avoids permission issues after installation.
        std::string logPath = "obsbot_sdk.log";
        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData) {
            std::filesystem::path logDir = std::filesystem::path(localAppData) / "PTZCommander";
            std::error_code ec;
            std::filesystem::create_directories(logDir, ec);
            if (!ec) {
                logPath = (logDir / "obsbot_sdk.log").string();
            }
        }

        logFile.open(logPath, std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            core::Logger::info("OBSBOT SDK logging redirected to: " + logPath);
            dev_set_log_handler(sdkLogHandler, nullptr);
        }
        else {
            core::Logger::error("Failed to open OBSBOT SDK log file for writing: " + logPath);
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

            int32_t scanResult = devices_.startNetworkScanImmediately();
            if (scanResult == RM_RET_OK) {
                core::Logger::info("Network scan started successfully");
            }
            else {
                core::Logger::warn("Network scan failed to start (code: " + std::to_string(scanResult) + ") - scanning may already be in progress");
            }

            findDevices();

            devices_.setDevChangedCallback(onDeviceChanged, this);

            core::Logger::info("ObsbotCameraAdapter started (USB + Network + SDK logging to file)");
        }
    }

    void ObsbotCameraAdapter::stop() {
        if (running_) {
            running_ = false;
            devices_.setDevChangedCallback(nullptr, nullptr);
            core::Logger::info("ObsbotCameraAdapter stopped");
        }
    }

    void ObsbotCameraAdapter::onDeviceChanged(std::string devSn, bool connected, void* param) {
        auto* adapter = static_cast<ObsbotCameraAdapter*>(param);
        if (!adapter) return;

        core::Logger::info("Device event: " + devSn + " - " + (connected ? "CONNECTED" : "DISCONNECTED"));

        if (connected) {
            auto dev = adapter->devices_.getDevBySn(devSn);
            if (dev) {
                if (dev->devMode() == Device::DevModeUvc ||
                    dev->devMode() == Device::DevModeNet) {

                    std::string mode = (dev->devMode() == Device::DevModeNet) ? "NETWORK" : "USB";
                    core::Logger::info("Adding " + mode + " device: " + dev->devName() + " (" + devSn + ")");

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
                core::Logger::info("Pre-existing device found: " + dev->devName());
                manager_.addCamera(std::make_shared<ObsbotCamera>(dev));
            }
        }
    }

} // namespace cameras::obsbot