#include "SetupDumpServer.h"
#include "Logger.h"
#include "../cameras/PresetStore.h"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

namespace core {

    SetupDumpServer::SetupDumpServer(cameras::CameraManager& cameraMgr,
                                     cameras::SceneStore& sceneStore)
        : cameraMgr_(cameraMgr),
          sceneStore_(sceneStore),
          server_(port_, host_) {
        server_.disablePerMessageDeflate();
        server_.setOnClientMessageCallback(
            [this](std::shared_ptr<ix::ConnectionState>,
                   ix::WebSocket& webSocket,
                   const ix::WebSocketMessagePtr& msg) {
                if (msg->type == ix::WebSocketMessageType::Open) {
                    webSocket.sendText(dumpJson());
                }
            });
    }

    SetupDumpServer::~SetupDumpServer() {
        stop();
    }

    bool SetupDumpServer::start() {
        if (started_) {
            return true;
        }

        if (!ix::initNetSystem()) {
            Logger::error("Setup WebSocket: failed to initialize network");
            return false;
        }
        netInitialized_ = true;

        auto res = server_.listen();
        if (!res.first) {
            Logger::error("Setup WebSocket listen failed on " + host_ + ":"
                          + std::to_string(port_) + ": " + res.second);
            ix::uninitNetSystem();
            netInitialized_ = false;
            return false;
        }

        server_.start();
        started_ = true;
        Logger::info("Setup WebSocket listening on ws://" + host_ + ":"
                     + std::to_string(port_));
        return true;
    }

    void SetupDumpServer::stop() {
        if (started_) {
            server_.stop();
            started_ = false;
        }
        if (netInitialized_) {
            ix::uninitNetSystem();
            netInitialized_ = false;
        }
    }

    void SetupDumpServer::broadcast() {
        if (!started_) {
            return;
        }
        const std::string payload = dumpJson();
        for (const auto& client : server_.getClients()) {
            if (client) {
                client->sendText(payload);
            }
        }
    }

    std::string SetupDumpServer::dumpJson() const {
        cameras::PresetStore store;
        return store.toJsonString(cameraMgr_, sceneStore_);
    }

}
