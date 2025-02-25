#include "Application.h"
#include <iostream>

namespace core {
    Application::Application()
        : obsbotAdapter_(std::make_unique<cameras::obsbot::ObsbotCameraAdapter>(cameraMgr_)),
        context_(cameraMgr_),
        commandHandler_(context_),
        running_(false) {
        adapterThread_ = std::thread(&cameras::obsbot::ObsbotCameraAdapter::start, obsbotAdapter_.get());
    }

    Application::~Application() {
        obsbotAdapter_->stop();  // Stops adapter’s loop
        if (adapterThread_.joinable()) {
            adapterThread_.join();  // Wait for adapter thread to finish
        }
    }

    void Application::start() {
        running_ = true;
        std::cout << StartMessage;
        std::string input;
        while (running_ && std::getline(std::cin, input)) {
            if (input == StopToken) {
                running_ = false;
            }
            else if (!input.empty()) {
                commandHandler_.execute(context_, input);
            }
            std::cout << Prompt;
        }
    }
}