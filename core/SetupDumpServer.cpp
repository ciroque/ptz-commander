#include "SetupDumpServer.h"
#include "Logger.h"
#include "../cameras/PresetStore.h"
#include "../commands/Arguments.h"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

namespace core {
    namespace {
        std::string replyOk(const std::string& name) {
            return nlohmann::json{
                {"ok", true},
                {"op", "scene.apply"},
                {"name", name}
            }.dump();
        }

        std::string replyError(const std::string& error) {
            return nlohmann::json{
                {"ok", false},
                {"error", error}
            }.dump();
        }

        bool hasWhitespace(const std::string& s) {
            return s.find_first_of(" \t\r\n") != std::string::npos;
        }
    }

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
                onClientMessage(webSocket, msg);
            });
    }

    SetupDumpServer::~SetupDumpServer() {
        stop();
    }

    void SetupDumpServer::setOnSceneApply(std::function<void(const std::string&)> cb) {
        onSceneApply_ = std::move(cb);
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

    void SetupDumpServer::onClientMessage(ix::WebSocket& webSocket,
                                          const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            webSocket.sendText(dumpJson());
            return;
        }
        if (msg->type != ix::WebSocketMessageType::Message) {
            return;
        }
        handleInbound(webSocket, msg->str);
    }

    bool SetupDumpServer::applyScene(ix::WebSocket& webSocket, const std::string& name) {
        if (name.empty() || hasWhitespace(name)) {
            webSocket.sendText(replyError("invalid scene name"));
            return false;
        }
        if (!onSceneApply_) {
            webSocket.sendText(replyError("scene apply is not available"));
            return false;
        }
        if (!sceneStore_.find(name)) {
            webSocket.sendText(replyError("Scene not found: " + name));
            return false;
        }
        Logger::info("WebSocket scene apply '" + name + "'");
        onSceneApply_(name);
        webSocket.sendText(replyOk(name));
        return true;
    }

    void SetupDumpServer::handleInbound(ix::WebSocket& webSocket, const std::string& payload) {
        const auto start = payload.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            webSocket.sendText(replyError("empty message"));
            return;
        }

        if (payload[start] == '{') {
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(payload);
            }
            catch (const nlohmann::json::parse_error&) {
                webSocket.sendText(replyError("invalid JSON"));
                return;
            }
            if (!j.is_object()) {
                webSocket.sendText(replyError("expected a JSON object"));
                return;
            }
            const std::string op = j.value("op", "");
            if (op != "scene.apply") {
                webSocket.sendText(replyError("unknown op"));
                return;
            }
            if (!j.contains("name") || !j["name"].is_string()) {
                webSocket.sendText(replyError("missing scene name"));
                return;
            }
            applyScene(webSocket, j["name"].get<std::string>());
            return;
        }

        const auto tokens = commands::splitArgs(payload);
        if (tokens.size() == 3 && tokens[0] == "scene" && tokens[1] == "apply") {
            applyScene(webSocket, tokens[2]);
            return;
        }

        webSocket.sendText(replyError("expected scene apply"));
    }

}
