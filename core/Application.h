#ifndef CORE_APPLICATION_H
#define CORE_APPLICATION_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include "../cameras/obsbot/ObsbotCameraAdapter.h"
#include "../cameras/visca/ViscaCameraAdapter.h"
#include "../commands/CommandHandler.h"
#include "../data/Context.h"
#include <memory>
#include <thread>

namespace core {
    class Application {
    private:
        cameras::CameraManager cameraMgr_;
        cameras::SceneStore sceneStore_;
        std::unique_ptr<cameras::obsbot::ObsbotCameraAdapter> obsbotAdapter_;
        std::unique_ptr<cameras::visca::ViscaCameraAdapter> viscaAdapter_;
        data::Context context_;
        commands::CommandHandler commandHandler_;
        std::thread obsbotAdapterThread_;
        std::thread viscaAdapterThread_;
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