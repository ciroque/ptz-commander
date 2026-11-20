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

using cameras::LoadStatus;
using cameras::PresetStore;
using cameras::onvif::kDpapiPasswordPrefix;
using cameras::onvif::protectPassword;

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

TEST(PresetStore, SkipsOneUndecryptableOnvifPasswordAndLoadsTheRest) {
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

    ASSERT_EQ(staged.onvif.size(), 1u);
    EXPECT_EQ(staged.onvif[0].host, "192.168.1.20");
    EXPECT_EQ(staged.onvif[0].user, "admin");
    EXPECT_EQ(staged.onvif[0].password, "good-secret");

    ASSERT_EQ(staged.onvifSkipped.size(), 1u);
    EXPECT_EQ(staged.onvifSkipped[0], "192.168.1.21");

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
