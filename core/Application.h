#ifndef CORE_APPLICATION_H
#define CORE_APPLICATION_H

#include "../cameras/CameraManager.h"
#include "../cameras/obsbot/ObsbotCameraAdapter.h"
#include "../commands/CommandHandler.h"
#include "../data/Context.h"
#include <memory>
#include <thread>

namespace core {
    class Application {
    private:
        cameras::CameraManager cameraMgr_;
        std::unique_ptr<cameras::obsbot::ObsbotCameraAdapter> obsbotAdapter_;
        data::Context context_;
        commands::CommandHandler commandHandler_;
        std::thread adapterThread_;  // Thread for adapter’s start()
        bool running_;
        const std::string StartMessage = "PTZ Commander (type 'exit' to quit)\n> ";
        const std::string Prompt = "> ";
        const std::string StopToken = "exit";

    public:
        Application();
        ~Application();
        void start();
    };
}

#endif // CORE_APPLICATION_H