#include <gtest/gtest.h>

#include "PresetStore.h"
#include "onvif/OnvifPassword.h"

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
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using cameras::CameraManager;
using cameras::LoadStatus;
using cameras::PresetStore;
using cameras::SceneStore;
using cameras::onvif::kDpapiPasswordPrefix;
using cameras::onvif::OnvifEndpoint;
using cameras::onvif::protectPassword;
using cameras::onvif::unprotectPassword;

namespace {

    struct TempPtzc {
        std::filesystem::path path;
        explicit TempPtzc(std::filesystem::path file) : path(std::move(file)) {}
        ~TempPtzc() {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
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

} // namespace

TEST(PresetStore, KeepsAnUndecryptableOnvifEntryForThePrompt) {
    const auto good = protectPassword("good-secret");
    ASSERT_TRUE(good);
    const std::string bad = undecryptablePassword();
    ASSERT_FALSE(bad.empty());

    nlohmann::json file = nlohmann::json::object();
    file["CAM1"] = {
        {"alias", "Pulpit"},
        {"presets", {{"wide", {{"name", "wide"}, {"pan", 1.5}, {"tilt", -2.0}, {"zoom", 10}}}}},
    };
    file["scenes"] = {{"sermon", {{"CAM1", "wide"}}}};
    file["onvif"] = nlohmann::json::array({
        {{"host", "192.168.1.20"},
         {"port", 80},
         {"user", "admin"},
         {"password", *good},
         {"path", "/onvif/device_service"}},
        {{"host", "192.168.1.21"},
         {"port", 80},
         {"user", "admin"},
         {"password", bad},
         {"path", "/onvif/device_service"}},
        {{"host", "192.168.1.22"}, {"port", 80}, {"user", "guest"}, {"path", "/onvif/device_service"}},
    });

    const TempPtzc temp(std::filesystem::temp_directory_path() / "ptz-onvif-load-one-bad.ptzc");
    {
        std::ofstream out(temp.path);
        ASSERT_TRUE(out);
        out << file.dump(2);
    }

    PresetStore store;
    cameras::StagedSetup staged;
    std::string error;
    const auto status = store.read(temp.path.string(), staged, &error);
    EXPECT_EQ(status, LoadStatus::Ok) << error;

    ASSERT_EQ(staged.onvif.size(), 3u);
    EXPECT_EQ(staged.onvif[0].host, "192.168.1.20");
    EXPECT_EQ(staged.onvif[0].user, "admin");
    EXPECT_EQ(staged.onvif[0].password, "good-secret");
    EXPECT_FALSE(staged.onvif[0].needsPassword);
    EXPECT_TRUE(staged.onvif[0].preservedPassword.empty());

    EXPECT_EQ(staged.onvif[1].host, "192.168.1.21");
    EXPECT_EQ(staged.onvif[1].user, "admin");
    EXPECT_TRUE(staged.onvif[1].password.empty());
    EXPECT_TRUE(staged.onvif[1].needsPassword);
    EXPECT_EQ(staged.onvif[1].preservedPassword, bad);

    EXPECT_EQ(staged.onvif[2].host, "192.168.1.22");
    EXPECT_TRUE(staged.onvif[2].password.empty());
    EXPECT_TRUE(staged.onvif[2].needsPassword);
    EXPECT_TRUE(staged.onvif[2].preservedPassword.empty());

    const auto camera = staged.cameras.find("CAM1");
    ASSERT_NE(camera, staged.cameras.end());
    ASSERT_EQ(camera->second.presets.size(), 1u);
    EXPECT_EQ(camera->second.presets[0].name, "wide");
    EXPECT_FLOAT_EQ(camera->second.presets[0].ptz.pan, 1.5f);
    EXPECT_EQ(camera->second.presets[0].ptz.zoom, 10);

    const auto scene = staged.scenes.find("sermon");
    ASSERT_NE(scene, staged.scenes.end());
    EXPECT_EQ(scene->second.bindings.at("CAM1"), "wide");
}

TEST(PresetStore, WritesAnUndecryptableBlobBack) {
    const std::string bad = undecryptablePassword();
    ASSERT_FALSE(bad.empty());

    OnvifEndpoint ep;
    ep.host = "192.168.1.21";
    ep.port = 80;
    ep.user = "admin";
    ep.servicePath = "/onvif/device_service";
    ep.needsPassword = true;
    ep.preservedPassword = bad;

    CameraManager mgr;
    SceneStore scenes;
    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-onvif-blob-save.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, {}, {}, {ep}, saved.path.string(), true));

    std::ifstream in(saved.path);
    ASSERT_TRUE(in.is_open());
    nlohmann::json savedJson;
    in >> savedJson;
    ASSERT_EQ(savedJson["onvif"].size(), 1u);
    EXPECT_EQ(savedJson["onvif"][0]["password"].get<std::string>(), bad);

    cameras::StagedSetup staged;
    std::string error;
    ASSERT_EQ(store.read(saved.path.string(), staged, &error), LoadStatus::Ok) << error;
    ASSERT_EQ(staged.onvif.size(), 1u);
    EXPECT_TRUE(staged.onvif[0].password.empty());
    EXPECT_TRUE(staged.onvif[0].needsPassword);
    EXPECT_EQ(staged.onvif[0].preservedPassword, bad);
}

TEST(PresetStore, ReplacesAPreservedBlobWithTheNewPassword) {
    const std::string bad = undecryptablePassword();
    ASSERT_FALSE(bad.empty());

    OnvifEndpoint ep;
    ep.host = "192.168.1.21";
    ep.port = 80;
    ep.user = "admin";
    ep.password = "new-secret";
    ep.preservedPassword = bad;

    CameraManager mgr;
    SceneStore scenes;
    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-onvif-blob-replace.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, {}, {}, {ep}, saved.path.string(), true));

    std::ifstream in(saved.path);
    ASSERT_TRUE(in.is_open());
    nlohmann::json savedJson;
    in >> savedJson;
    ASSERT_EQ(savedJson["onvif"].size(), 1u);
    const auto written = savedJson["onvif"][0]["password"].get<std::string>();
    EXPECT_EQ(written.find(bad), std::string::npos);
    const auto opened = unprotectPassword(written);
    ASSERT_TRUE(opened);
    EXPECT_EQ(*opened, "new-secret");
}

TEST(PresetStore, ExportOmitsAPreservedBlob) {
    const std::string bad = undecryptablePassword();
    ASSERT_FALSE(bad.empty());

    OnvifEndpoint ep;
    ep.host = "192.168.1.21";
    ep.user = "admin";
    ep.password = "new-secret";
    ep.preservedPassword = bad;

    CameraManager mgr;
    SceneStore scenes;
    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-onvif-blob-export.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, {}, {}, {ep}, saved.path.string(), false));

    std::ifstream in(saved.path);
    ASSERT_TRUE(in.is_open());
    nlohmann::json savedJson;
    in >> savedJson;
    ASSERT_EQ(savedJson["onvif"].size(), 1u);
    EXPECT_FALSE(savedJson["onvif"][0].contains("password"));
    EXPECT_EQ(savedJson["onvif"][0]["host"], "192.168.1.21");
}
