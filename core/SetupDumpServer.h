#ifndef CORE_SETUPDUMPSERVER_H
#define CORE_SETUPDUMPSERVER_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include <atomic>

namespace core {
    class SetupDumpServer {
    public:
        SetupDumpServer(cameras::CameraManager& cameraMgr, cameras::SceneStore& sceneStore);
        ~SetupDumpServer();

        SetupDumpServer(const SetupDumpServer&) = delete;
        SetupDumpServer& operator=(const SetupDumpServer&) = delete;

        bool start();
        void stop();
        void broadcast();

    private:
        std::string dumpJson() const;

        cameras::CameraManager& cameraMgr_;
        cameras::SceneStore& sceneStore_;
        ix::WebSocketServer server_;
        std::atomic<bool> started_{false};
        bool netInitialized_{false};
    };
}

#endif // CORE_SETUPDUMPSERVER_H
