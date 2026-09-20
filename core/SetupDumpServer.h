#ifndef CORE_SETUPDUMPSERVER_H
#define CORE_SETUPDUMPSERVER_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include <atomic>
#include <functional>
#include <string>

namespace core {
    class SetupDumpServer {
    public:
        SetupDumpServer(cameras::CameraManager& cameraMgr, cameras::SceneStore& sceneStore);
        ~SetupDumpServer();

        SetupDumpServer(const SetupDumpServer&) = delete;
        SetupDumpServer& operator=(const SetupDumpServer&) = delete;

        void setOnSceneApply(std::function<void(const std::string&)> cb);

        bool start();
        void stop();
        void broadcast();

    private:
        std::string dumpJson() const;
        void onClientMessage(ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg);
        void handleInbound(ix::WebSocket& webSocket, const std::string& payload);
        bool applyScene(ix::WebSocket& webSocket, const std::string& name);

        cameras::CameraManager& cameraMgr_;
        cameras::SceneStore& sceneStore_;
        std::function<void(const std::string&)> onSceneApply_;
        const std::string host_{"127.0.0.1"};
        const int port_{7420};
        ix::WebSocketServer server_;
        std::atomic<bool> started_{false};
        bool netInitialized_{false};
    };
}

#endif // CORE_SETUPDUMPSERVER_H
