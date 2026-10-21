#include "Application.h"
#include "ptz_version.h"

#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace core {
    Application::Application()
        : obsbotAdapter_(std::make_unique<cameras::obsbot::ObsbotCameraAdapter>(cameraMgr_)),
          viscaAdapter_(std::make_unique<cameras::visca::ViscaCameraAdapter>(cameraMgr_)),
          context_(cameraMgr_, sceneStore_),
          commandHandler_(),
          sceneControlServer_(cameraMgr_, sceneStore_),
          running_(false) {
        context_.onSetupChanged = [this] { sceneControlServer_.broadcast(); };
        context_.addViscaIp = [this](const cameras::visca::ViscaIpEndpoint& endpoint, std::string& error) {
            const bool ok = viscaAdapter_->addIpCamera(endpoint, error);
            context_.viscaIpEndpoints = viscaAdapter_->ipEndpoints();
            return ok;
        };
        context_.loadViscaIp = [this](const cameras::visca::ViscaIpEndpoint& endpoint) {
            const bool ok = viscaAdapter_->loadIpCamera(endpoint);
            context_.viscaIpEndpoints = viscaAdapter_->ipEndpoints();
            return ok;
        };
        context_.calibrateViscaIp = [this](const std::string& serial,
                                           const cameras::visca::CalibrationRanges& ranges,
                                           const cameras::visca::ViscaCalibrator::Progress& progress,
                                           std::string& error) {
            auto profile = viscaAdapter_->calibrateIpCamera(serial, ranges, progress, error);
            context_.viscaIpEndpoints = viscaAdapter_->ipEndpoints();
            return profile;
        };
        context_.addViscaSerial = [this](const cameras::visca::ViscaSerialEndpoint& endpoint, std::string& error) {
            const bool ok = viscaAdapter_->addSerialCamera(endpoint, error);
            context_.viscaSerialEndpoints = viscaAdapter_->serialEndpoints();
            return ok;
        };
        context_.loadViscaSerial = [this](const cameras::visca::ViscaSerialEndpoint& endpoint) {
            const bool ok = viscaAdapter_->loadSerialCamera(endpoint);
            context_.viscaSerialEndpoints = viscaAdapter_->serialEndpoints();
            return ok;
        };
        sceneControlServer_.setOnSceneApply([this](const std::string& name) {
            std::lock_guard<std::mutex> lock(commandMutex_);
            context_.out << std::endl;
            commandHandler_.execute(context_, "scene apply " + name);
            context_.out << Prompt << std::flush;
        });

        // Start OBSBOT adapter (hotplug + network scan) in its own thread
        obsbotAdapterThread_ = std::thread(&cameras::obsbot::ObsbotCameraAdapter::start, obsbotAdapter_.get());
        // VISCA cameras aren't discovered: they come from `camera add` and `setup load`.
    }

    Application::~Application() {
        sceneControlServer_.stop();

        if (obsbotAdapter_) {
            obsbotAdapter_->stop();
        }

        if (obsbotAdapterThread_.joinable()) {
            obsbotAdapterThread_.join();
        }
    }

    void Application::start() {
        sceneControlServer_.start();
        running_ = true;
        context_.out << "PTZ Commander " << PTZ_VERSION_PRODUCT
                     << " (type 'exit' to quit)\n> ";
        std::string input;
        while (running_) {
            if (!std::getline(std::cin, input)) {
#ifdef _WIN32
                DWORD consoleMode = 0;
                const bool stdinIsConsole =
                    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &consoleMode) != 0;
                if (stdinIsConsole) {
                    std::cin.clear();
                    continue;
                }
#endif
                break;
            }
            if (input == StopToken) {
                running_ = false;
            }
            else {
                std::lock_guard<std::mutex> lock(commandMutex_);
                if (!input.empty()) {
                    commandHandler_.execute(context_, input);
                }
                context_.out << Prompt << std::flush;
            }
        }
    }
}