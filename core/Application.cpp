#include "Application.h"
#include "../cameras/obsbot/ObsbotCamera.h"
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
          onvifAdapter_(std::make_unique<cameras::onvif::OnvifCameraAdapter>(cameraMgr_)),
          context_(cameraMgr_, sceneStore_), commandHandler_(), sceneControlServer_(cameraMgr_, sceneStore_),
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
        context_.calibrateVisca
            = [this](const std::string& serial, const cameras::visca::CalibrationRanges& ranges,
                     const cameras::visca::ViscaCalibrator::Progress& progress, std::string& error) {
                  auto profile = viscaAdapter_->calibrateCamera(serial, ranges, progress, error);
                  context_.viscaIpEndpoints = viscaAdapter_->ipEndpoints();
                  context_.viscaSerialEndpoints = viscaAdapter_->serialEndpoints();
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
        context_.addOnvif = [this](const cameras::onvif::OnvifEndpoint& endpoint) {
            const auto result = onvifAdapter_->add(endpoint);
            context_.onvifEndpoints = onvifAdapter_->endpoints();
            return result;
        };
        context_.addInactiveOnvif = [this](const cameras::onvif::OnvifEndpoint& endpoint) {
            const auto result = onvifAdapter_->addInactive(endpoint);
            context_.onvifEndpoints = onvifAdapter_->endpoints();
            return result;
        };
        context_.loginOnvif = [this](const std::string& serial, const std::string& password) {
            const auto result = onvifAdapter_->login(serial, password);
            context_.onvifEndpoints = onvifAdapter_->endpoints();
            return result;
        };
        context_.discardCamera = [this](const std::string& serial, bool commit) {
            auto camera = cameraMgr_.findBySerial(serial);
            if (!camera) {
                return data::Context::DiscardCameraStatus::NotFound;
            }
            if (dynamic_cast<cameras::ObsbotCamera*>(camera.get())) {
                return data::Context::DiscardCameraStatus::Obsbot;
            }
            if (!commit) {
                return data::Context::DiscardCameraStatus::Removed;
            }
            viscaAdapter_->discard(serial);
            onvifAdapter_->discard(serial);
            if (cameraMgr_.findBySerial(serial)) {
                cameraMgr_.removeCamera(serial);
            }
            context_.viscaIpEndpoints = viscaAdapter_->ipEndpoints();
            context_.viscaSerialEndpoints = viscaAdapter_->serialEndpoints();
            context_.onvifEndpoints = onvifAdapter_->endpoints();
            return data::Context::DiscardCameraStatus::Removed;
        };
        sceneControlServer_.setOnSceneApply([this](const std::string& name) {
            std::lock_guard<std::mutex> lock(commandMutex_);
            context_.out << std::endl;
            commandHandler_.execute(context_, "scene apply " + name);
            context_.out << Prompt << std::flush;
        });

        // The wait runs here. commandMutex_ is held only around one move.
        tourRunner_ = std::make_unique<commands::TourRunner>(commandMutex_);
        context_.tour = tourRunner_.get();

        // Start OBSBOT adapter (hotplug + network scan) in its own thread
        obsbotAdapterThread_ = std::thread(&cameras::obsbot::ObsbotCameraAdapter::start, obsbotAdapter_.get());
    }

    Application::~Application() {
        // Join before the cameras go away, and not while holding commandMutex_.
        if (tourRunner_) {
            tourRunner_->shutdown();
            context_.tour = nullptr;
        }

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
        context_.out << "PTZ Commander " << PTZ_VERSION_PRODUCT << " (type 'exit' to quit)\n> ";
        std::string input;
        while (running_) {
            if (!std::getline(std::cin, input)) {
#ifdef _WIN32
                DWORD consoleMode = 0;
                const bool stdinIsConsole = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &consoleMode) != 0;
                if (stdinIsConsole) {
                    std::cin.clear();
                    continue;
                }
#endif
                break;
            }
            if (input == StopToken) {
                running_ = false;
            } else {
                std::lock_guard<std::mutex> lock(commandMutex_);
                if (!input.empty()) {
                    commandHandler_.execute(context_, input);
                }
                context_.out << Prompt << std::flush;
            }
        }
    }
} // namespace core
