#include "Application.h"
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
          setupDumpServer_(cameraMgr_, sceneStore_),
          running_(false) {
        context_.onSetupChanged = [this] { setupDumpServer_.broadcast(); };
        setupDumpServer_.setOnSceneApply([this](const std::string& name) {
            std::lock_guard<std::mutex> lock(commandMutex_);
            commandHandler_.execute(context_, "scene apply " + name);
        });

        // Start OBSBOT adapter (hotplug + network scan) in its own thread
        obsbotAdapterThread_ = std::thread(&cameras::obsbot::ObsbotCameraAdapter::start, obsbotAdapter_.get());

        // Start VISCA serial discovery adapter (Keyspan priority + other COM ports)
        viscaAdapterThread_ = std::thread(&cameras::visca::ViscaCameraAdapter::start, viscaAdapter_.get());
    }

    Application::~Application() {
        setupDumpServer_.stop();

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
        setupDumpServer_.start();
        running_ = true;
        context_.out << StartMessage;
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
            else if (!input.empty()) {
                std::lock_guard<std::mutex> lock(commandMutex_);
                commandHandler_.execute(context_, input);
            }
            context_.out << Prompt;
        }
    }
}