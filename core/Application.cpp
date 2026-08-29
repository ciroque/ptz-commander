#include "Application.h"
#include <iostream>

namespace core {
    Application::Application()
        : obsbotAdapter_(std::make_unique<cameras::obsbot::ObsbotCameraAdapter>(cameraMgr_)),
          viscaAdapter_(std::make_unique<cameras::visca::ViscaCameraAdapter>(cameraMgr_)),
          context_(cameraMgr_, sceneStore_),
          commandHandler_(),
          running_(false) {
        // Start OBSBOT adapter (hotplug + network scan) in its own thread
        obsbotAdapterThread_ = std::thread(&cameras::obsbot::ObsbotCameraAdapter::start, obsbotAdapter_.get());

        // Start VISCA serial discovery adapter (Keyspan priority + other COM ports)
        viscaAdapterThread_ = std::thread(&cameras::visca::ViscaCameraAdapter::start, viscaAdapter_.get());
    }

    Application::~Application() {
        if (viscaAdapter_) {
            viscaAdapter_->stop();
        }
        if (obsbotAdapter_) {
            obsbotAdapter_->stop();
        }

        if (viscaAdapterThread_.joinable()) {
            viscaAdapterThread_.join();
        }
        if (obsbotAdapterThread_.joinable()) {
            obsbotAdapterThread_.join();
        }
    }

    void Application::start() {
        running_ = true;
        context_.out << StartMessage;
        std::string input;
        while (running_ && std::getline(std::cin, input)) {
            if (input == StopToken) {
                running_ = false;
            }
            else if (!input.empty()) {
                commandHandler_.execute(context_, input);
            }
            context_.out << Prompt;
        }
    }
}