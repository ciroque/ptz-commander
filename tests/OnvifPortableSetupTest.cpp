#include <gtest/gtest.h>

#include "CameraManager.h"
#include "PresetStore.h"
#include "SceneStore.h"
#include "onvif/OnvifPassword.h"
#include "setup/ExportCommand.h"
#include "setup/LoadCommand.h"

#include "../data/Context.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
using cameras::PresetStore;
using cameras::Ptz;
using cameras::SceneStore;
using cameras::onvif::kDpapiPasswordPrefix;
using cameras::onvif::OnvifAddResult;
using cameras::onvif::OnvifAddStatus;
using cameras::onvif::OnvifEndpoint;
using cameras::onvif::protectPassword;

namespace {

    struct TempPtzc {
        std::filesystem::path path;
        explicit TempPtzc(std::filesystem::path file) : path(std::move(file)) {}
        TempPtzc(const TempPtzc&) = delete;
        TempPtzc& operator=(const TempPtzc&) = delete;
        TempPtzc(TempPtzc&& other) noexcept : path(std::move(other.path)) { other.path.clear(); }
        TempPtzc& operator=(TempPtzc&&) = delete;
        ~TempPtzc() {
            if (path.empty()) {
                return;
            }
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    };

    class ShelfCamera : public Camera {
    public:
        explicit ShelfCamera(std::string serial) : serial_(std::move(serial)) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return connected_; }
        bool setPosition(float, float, int) override { return connected_; }
        bool setZoom(int, int) override { return connected_; }
        Ptz getCurrentPtz() const override { return {}; }
        void connect() { connected_ = true; }

    private:
        std::string serial_;
        bool connected_ = false;
    };

    std::string undecryptablePassword() {
        const char raw[] = "not-a-real-dpapi-blob";
        DWORD chars = 0;
        if (!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(raw), sizeof(raw) - 1,
                                  CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &chars)
            || chars == 0) {
            return {};
        }
        std::string encoded(chars, '\0');
        if (!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(raw), sizeof(raw) - 1,
                                  CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded.data(), &chars)) {
            return {};
        }
        while (!encoded.empty() && encoded.back() == '\0') {
            encoded.pop_back();
        }
        return std::string(kDpapiPasswordPrefix) + encoded;
    }

    TempPtzc writePtzc(const std::filesystem::path& path, const nlohmann::json& body) {
        TempPtzc temp(path);
        std::ofstream out(temp.path);
        EXPECT_TRUE(out.is_open());
        out << body.dump(2);
        return temp;
    }

    nlohmann::json onvifEntry(const std::string& host, const std::string& user, const nlohmann::json& password) {
        nlohmann::json entry = {{"host", host}, {"port", 80}, {"user", user}, {"path", "/onvif/device_service"}};
        if (!password.is_null()) {
            entry["password"] = password;
        }
        return entry;
    }

    struct LoadHarness {
        CameraManager mgr;
        SceneStore scenes;
        std::ostringstream out;
        std::ostringstream err;
        data::Context ctx;
        std::vector<OnvifEndpoint> connected;
        std::vector<OnvifEndpoint> inactive;
        std::function<OnvifAddResult(const OnvifEndpoint&)> connect;

        LoadHarness() : ctx(mgr, scenes, out, err) {
            ctx.addOnvif = [this](const OnvifEndpoint& ep) {
                if (connect) {
                    return connect(ep);
                }
                connected.push_back(ep);
                auto camera = std::make_shared<ShelfCamera>(ep.serial());
                camera->connect();
                mgr.addCamera(camera);
                ctx.onvifEndpoints.push_back(ep);
                OnvifAddResult result;
                result.status = OnvifAddStatus::Added;
                return result;
            };
            ctx.addInactiveOnvif = [this](const OnvifEndpoint& ep) {
                inactive.push_back(ep);
                mgr.addCamera(std::make_shared<ShelfCamera>(ep.serial()));
                ctx.onvifEndpoints.push_back(ep);
                OnvifAddResult result;
                result.status = OnvifAddStatus::Added;
                return result;
            };
        }
    };

    OnvifAddResult authRejected() {
        OnvifAddResult result;
        result.status = OnvifAddStatus::Failed;
        result.authRejected = true;
        result.detail = "the camera rejected the username or password";
        return result;
    }

} // namespace

TEST(PresetStore, ExportOmitsPasswordsAndOtherwiseMatchesSave) {
    CameraManager mgr;
    SceneStore scenes;
    auto camera = std::make_shared<ShelfCamera>("ONVIF:192.168.1.20:80");
    camera->setAlias("Pulpit");
    camera->addPreset("wide", Preset("wide", Ptz{1.5f, -2.0f, 10}));
    mgr.addCamera(camera);
    scenes.upsertBinding("sermon", camera->getSerialNumber(), "wide");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    cameras::visca::ViscaIpEndpoint ip;
    ip.host = "192.168.1.30";
    ctx.viscaIpEndpoints = {ip};
    ctx.onvifEndpoints
        = {OnvifEndpoint{"192.168.1.20", 80, "/onvif/device_service", "admin", "local-secret", false, ""},
           OnvifEndpoint{"192.168.1.21", 80, "/onvif/ptz", "operator", "other-secret", false, ""}};

    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-export-saved.ptzc");
    const TempPtzc exported(std::filesystem::temp_directory_path() / "ptz-export-stripped.ptzc");

    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, ctx.viscaIpEndpoints, {}, ctx.onvifEndpoints, saved.path.string(), true));

    commands::setup::ExportCommand command;
    command.execute(ctx, exported.path.string());
    EXPECT_TRUE(err.str().empty());

    std::ifstream savedIn(saved.path);
    std::ifstream exportedIn(exported.path);
    ASSERT_TRUE(savedIn.is_open());
    ASSERT_TRUE(exportedIn.is_open());
    nlohmann::json savedJson;
    nlohmann::json exportedJson;
    savedIn >> savedJson;
    exportedIn >> exportedJson;

    ASSERT_TRUE(savedJson.contains("onvif"));
    ASSERT_EQ(savedJson["onvif"].size(), 2u);
    for (const auto& entry : savedJson["onvif"]) {
        ASSERT_TRUE(entry.contains("password"));
        EXPECT_EQ(entry["password"].get<std::string>().rfind(kDpapiPasswordPrefix, 0), 0u);
    }
    for (const auto& entry : exportedJson["onvif"]) {
        EXPECT_FALSE(entry.contains("password"));
    }

    for (auto& entry : savedJson["onvif"]) {
        entry.erase("password");
    }
    EXPECT_EQ(savedJson, exportedJson);
}

TEST(SetupLoad, AsksOncePerOnvifCameraInAnExport) {
    nlohmann::json file = nlohmann::json::object();
    file["onvif"] = nlohmann::json::array({
        onvifEntry("192.168.1.20", "admin", nullptr),
        onvifEntry("192.168.1.21", "operator", nullptr),
    });
    const TempPtzc temp = writePtzc(std::filesystem::temp_directory_path() / "ptz-export-load.ptzc", file);

    LoadHarness harness;
    int prompts = 0;
    std::vector<std::string> hosts;
    commands::setup::LoadCommand command([&](const OnvifEndpoint& ep) {
        ++prompts;
        hosts.push_back(ep.user + "@" + ep.host);
        return std::optional<std::string>("secret");
    });
    command.execute(harness.ctx, temp.path.string());

    EXPECT_EQ(prompts, 2);
    EXPECT_EQ(hosts, (std::vector<std::string>{"admin@192.168.1.20", "operator@192.168.1.21"}));
    ASSERT_EQ(harness.connected.size(), 2u);
    EXPECT_EQ(harness.connected[0].password, "secret");
    EXPECT_EQ(harness.connected[1].password, "secret");
    EXPECT_TRUE(harness.inactive.empty());
}

TEST(SetupLoad, BlankAnswerKeepsTheCameraInactiveThroughSave) {
    const std::string serial = "ONVIF:192.168.1.21:80";
    nlohmann::json file = nlohmann::json::object();
    file[serial] = {
        {"alias", "Pulpit"},
        {"presets", {{"wide", {{"name", "wide"}, {"pan", 1.5}, {"tilt", -2.0}, {"zoom", 10}}}}},
    };
    file["scenes"] = {{"sermon", {{serial, "wide"}}}};
    file["onvif"] = nlohmann::json::array({onvifEntry("192.168.1.21", "admin", nullptr)});
    const TempPtzc loaded = writePtzc(std::filesystem::temp_directory_path() / "ptz-blank-load.ptzc", file);

    LoadHarness harness;
    int prompts = 0;
    commands::setup::LoadCommand command([&](const OnvifEndpoint&) {
        ++prompts;
        return std::optional<std::string>{};
    });
    command.execute(harness.ctx, loaded.path.string());

    EXPECT_EQ(prompts, 1);
    EXPECT_TRUE(harness.connected.empty());
    ASSERT_EQ(harness.inactive.size(), 1u);
    EXPECT_TRUE(harness.inactive[0].password.empty());
    EXPECT_NE(harness.err.str().find("left inactive"), std::string::npos);

    const auto camera = harness.mgr.findBySerial(serial);
    ASSERT_NE(camera, nullptr);
    EXPECT_FALSE(camera->isConnected());
    EXPECT_EQ(camera->getAlias(), "Pulpit");
    const auto* preset = camera->getPresetByName("wide");
    ASSERT_NE(preset, nullptr);
    EXPECT_FLOAT_EQ(preset->ptz.pan, 1.5f);
    EXPECT_EQ(preset->ptz.zoom, 10);
    const auto* scene = harness.scenes.find("sermon");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->bindings.at(serial), "wide");

    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-blank-save.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(harness.mgr, harness.scenes, {}, {}, harness.ctx.onvifEndpoints, saved.path.string()));

    std::ifstream savedIn(saved.path);
    ASSERT_TRUE(savedIn.is_open());
    nlohmann::json savedJson;
    savedIn >> savedJson;
    ASSERT_EQ(savedJson["onvif"].size(), 1u);
    EXPECT_FALSE(savedJson["onvif"][0].contains("password"));
    EXPECT_EQ(savedJson["onvif"][0]["host"], "192.168.1.21");
    EXPECT_EQ(savedJson["onvif"][0]["user"], "admin");
    EXPECT_EQ(savedJson["onvif"][0]["port"], 80);

    cameras::StagedSetup staged;
    std::string error;
    ASSERT_EQ(store.read(saved.path.string(), staged, &error), cameras::LoadStatus::Ok) << error;
    ASSERT_EQ(staged.onvif.size(), 1u);
    EXPECT_TRUE(staged.onvif[0].needsPassword);
    EXPECT_TRUE(staged.onvif[0].password.empty());
    const auto savedCamera = staged.cameras.find(serial);
    ASSERT_NE(savedCamera, staged.cameras.end());
    ASSERT_EQ(savedCamera->second.presets.size(), 1u);
    EXPECT_EQ(savedCamera->second.presets[0].name, "wide");
    EXPECT_EQ(staged.scenes.at("sermon").bindings.at(serial), "wide");
}

TEST(SetupLoad, UndecryptableBlobUsesThePrompt) {
    const auto good = protectPassword("good-secret");
    ASSERT_TRUE(good);
    const std::string bad = undecryptablePassword();
    ASSERT_FALSE(bad.empty());

    nlohmann::json file = nlohmann::json::object();
    file["onvif"] = nlohmann::json::array({
        onvifEntry("192.168.1.20", "admin", *good),
        onvifEntry("192.168.1.21", "operator", bad),
    });
    const TempPtzc temp = writePtzc(std::filesystem::temp_directory_path() / "ptz-bad-blob-load.ptzc", file);

    LoadHarness harness;
    int prompts = 0;
    std::string asked;
    commands::setup::LoadCommand command([&](const OnvifEndpoint& ep) {
        ++prompts;
        asked = ep.user + "@" + ep.host;
        return std::optional<std::string>("from-prompt");
    });
    command.execute(harness.ctx, temp.path.string());

    EXPECT_EQ(prompts, 1);
    EXPECT_EQ(asked, "operator@192.168.1.21");
    ASSERT_EQ(harness.connected.size(), 2u);
    EXPECT_EQ(harness.connected[0].host, "192.168.1.20");
    EXPECT_EQ(harness.connected[0].password, "good-secret");
    EXPECT_EQ(harness.connected[1].host, "192.168.1.21");
    EXPECT_EQ(harness.connected[1].password, "from-prompt");
    EXPECT_TRUE(harness.inactive.empty());
}

TEST(SetupLoad, OlderPasswordsLoadWithoutAPrompt) {
    const auto wrapped = protectPassword("dpapi-secret");
    ASSERT_TRUE(wrapped);

    nlohmann::json file = nlohmann::json::object();
    file["onvif"] = nlohmann::json::array({
        onvifEntry("192.168.1.20", "admin", "plain-secret"),
        onvifEntry("192.168.1.21", "operator", *wrapped),
    });
    const TempPtzc temp = writePtzc(std::filesystem::temp_directory_path() / "ptz-legacy-load.ptzc", file);

    LoadHarness harness;
    int prompts = 0;
    commands::setup::LoadCommand command([&](const OnvifEndpoint&) {
        ++prompts;
        return std::optional<std::string>("should-not-be-asked");
    });
    command.execute(harness.ctx, temp.path.string());

    EXPECT_EQ(prompts, 0);
    EXPECT_TRUE(harness.inactive.empty());
    ASSERT_EQ(harness.connected.size(), 2u);
    EXPECT_EQ(harness.connected[0].password, "plain-secret");
    EXPECT_EQ(harness.connected[1].password, "dpapi-secret");
}

TEST(SetupLoad, AuthFaultAsksAgainThenLeavesTheCameraInactive) {
    nlohmann::json file = nlohmann::json::object();
    file["onvif"] = nlohmann::json::array({onvifEntry("192.168.1.21", "admin", nullptr)});
    const TempPtzc temp = writePtzc(std::filesystem::temp_directory_path() / "ptz-auth-retry.ptzc", file);

    LoadHarness harness;
    int prompts = 0;
    harness.connect = [&](const OnvifEndpoint& ep) {
        harness.connected.push_back(ep);
        return authRejected();
    };
    commands::setup::LoadCommand command([&](const OnvifEndpoint&) {
        ++prompts;
        return std::optional<std::string>("nope");
    });
    command.execute(harness.ctx, temp.path.string());

    EXPECT_EQ(prompts, 3);
    ASSERT_EQ(harness.connected.size(), 3u);
    EXPECT_EQ(harness.connected[0].password, "nope");
    ASSERT_EQ(harness.inactive.size(), 1u);
    EXPECT_TRUE(harness.inactive[0].password.empty());
    const auto camera = harness.mgr.findBySerial("ONVIF:192.168.1.21:80");
    ASSERT_NE(camera, nullptr);
    EXPECT_FALSE(camera->isConnected());
}

TEST(SetupLoad, ACameraThatDoesNotAnswerIsLeftOut) {
    nlohmann::json file = nlohmann::json::object();
    file["onvif"] = nlohmann::json::array({onvifEntry("192.168.1.21", "admin", "known-secret")});
    const TempPtzc temp = writePtzc(std::filesystem::temp_directory_path() / "ptz-offline-load.ptzc", file);

    LoadHarness harness;
    int prompts = 0;
    harness.connect = [&](const OnvifEndpoint& ep) {
        harness.connected.push_back(ep);
        OnvifAddResult result;
        result.status = OnvifAddStatus::Failed;
        result.detail = "the camera did not answer";
        return result;
    };
    commands::setup::LoadCommand command([&](const OnvifEndpoint&) {
        ++prompts;
        return std::optional<std::string>("should-not-be-asked");
    });
    command.execute(harness.ctx, temp.path.string());

    EXPECT_EQ(prompts, 0);
    ASSERT_EQ(harness.connected.size(), 1u);
    EXPECT_EQ(harness.connected[0].password, "known-secret");
    EXPECT_TRUE(harness.inactive.empty());
    EXPECT_EQ(harness.mgr.getCameras().size(), 0u);
    EXPECT_NE(harness.err.str().find("did not answer"), std::string::npos);
}
