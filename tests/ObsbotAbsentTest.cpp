#include <gtest/gtest.h>

#include "AbsentObsbot.h"
#include "CameraManager.h"
#include "ConnectionLabel.h"
#include "PresetStore.h"
#include "SceneStore.h"
#include "camera/DiscardCommand.h"
#include "scene/ApplyCommand.h"
#include "setup/LoadCommand.h"

#include "../data/Context.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
using cameras::PresetStore;
using cameras::Ptz;

namespace {

    struct TempPtzc {
        std::filesystem::path path;
        explicit TempPtzc(std::filesystem::path file) : path(std::move(file)) {}
        TempPtzc(const TempPtzc&) = delete;
        TempPtzc& operator=(const TempPtzc&) = delete;
        ~TempPtzc() {
            if (path.empty()) {
                return;
            }
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    };

    class StandIn : public Camera {
    public:
        explicit StandIn(std::string serial) : serial_(std::move(serial)) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return true; }
        bool setPosition(float pan, float tilt, int zoom) override {
            moved_ = Ptz{pan, tilt, zoom};
            didMove_ = true;
            return true;
        }
        bool setZoom(int, int) override { return false; }
        Ptz getCurrentPtz() const override { return moved_; }

        bool didMove() const { return didMove_; }

    private:
        std::string serial_;
        Ptz moved_{};
        bool didMove_ = false;
    };

    void writeFile(const std::filesystem::path& path, const std::string& text) {
        std::ofstream out(path);
        ASSERT_TRUE(out.is_open());
        out << text;
    }

    void wireDiscard(data::Context& ctx, CameraManager& mgr) {
        ctx.discardCamera = [&](const std::string& serial, bool commit) {
            if (!mgr.findBySerial(serial)) {
                return data::Context::DiscardCameraStatus::NotFound;
            }
            if (!commit) {
                return data::Context::DiscardCameraStatus::Removed;
            }
            mgr.removeCamera(serial);
            return data::Context::DiscardCameraStatus::Removed;
        };
    }

    const std::string kSerial = "RMOWTHF7211JGR";

    const char* kFile = R"({
  "RMOWTHF7211JGR": {
    "alias": "pulpit",
    "presets": {
      "wide": { "name": "wide", "pan": 1.5, "tilt": -2.0, "zoom": 40 }
    }
  },
  "VISCA-IP:192.168.1.40:52381": {
    "alias": "not-obsbot",
    "presets": { "wide": { "name": "wide", "pan": 0.0, "tilt": 0.0, "zoom": 0 } }
  },
  "ONVIF:192.168.1.41:80": {
    "alias": "also-not",
    "presets": { "wide": { "name": "wide", "pan": 0.0, "tilt": 0.0, "zoom": 0 } }
  },
  "scenes": { "sermon": { "RMOWTHF7211JGR": "wide" } }
})";

} // namespace

TEST(ObsbotAbsent, SerialPrefixes) {
    EXPECT_TRUE(cameras::isObsbotSerial(kSerial));
    EXPECT_FALSE(cameras::isObsbotSerial("VISCA-IP:192.168.1.40:52381"));
    EXPECT_FALSE(cameras::isObsbotSerial("VISCA:COM7:1"));
    EXPECT_FALSE(cameras::isObsbotSerial("ONVIF:192.168.1.41:80"));
}

TEST(ObsbotAbsent, LoadKeepsAnUnpluggedCameraInTheNextSave) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);

    const TempPtzc file(std::filesystem::temp_directory_path() / "ptz-obsbot-absent-load.ptzc");
    writeFile(file.path, kFile);

    commands::setup::LoadCommand command([](const cameras::onvif::OnvifEndpoint&) -> std::optional<std::string> {
        ADD_FAILURE() << "an OBSBOT file should not ask for a password";
        return std::nullopt;
    });
    command.execute(ctx, file.path.string());

    EXPECT_EQ(err.str(), "");
    auto camera = mgr.findBySerial(kSerial);
    ASSERT_NE(camera, nullptr);
    EXPECT_NE(dynamic_cast<cameras::AbsentObsbotCamera*>(camera.get()), nullptr);
    EXPECT_EQ(camera->getAlias(), "pulpit");
    ASSERT_NE(camera->getPresetByName("wide"), nullptr);
    EXPECT_FLOAT_EQ(camera->getPresetByName("wide")->ptz.pan, 1.5f);
    EXPECT_EQ(camera->getPresetByName("wide")->ptz.zoom, 40);
    EXPECT_EQ(camera->connectionState(), cameras::ConnectionState::Unplugged);
    EXPECT_STREQ(commands::connectionLabel(*camera), "Offline");
    EXPECT_EQ(mgr.findBySerial("VISCA-IP:192.168.1.40:52381"), nullptr);
    EXPECT_EQ(mgr.findBySerial("ONVIF:192.168.1.41:80"), nullptr);
    ASSERT_NE(scenes.find("sermon"), nullptr);
    EXPECT_EQ(scenes.find("sermon")->bindings.at(kSerial), "wide");

    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-obsbot-absent-saved.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, saved.path.string()));
    cameras::StagedSetup staged;
    ASSERT_EQ(store.read(saved.path.string(), staged), cameras::LoadStatus::Ok);
    const auto kept = staged.cameras.find(kSerial);
    ASSERT_NE(kept, staged.cameras.end());
    ASSERT_TRUE(kept->second.alias);
    EXPECT_EQ(*kept->second.alias, "pulpit");
    ASSERT_EQ(kept->second.presets.size(), 1u);
    EXPECT_EQ(kept->second.presets[0].name, "wide");
    EXPECT_FLOAT_EQ(kept->second.presets[0].ptz.pan, 1.5f);
    EXPECT_EQ(kept->second.presets[0].ptz.zoom, 40);
    EXPECT_EQ(staged.scenes.at("sermon").bindings.at(kSerial), "wide");
    EXPECT_EQ(staged.cameras.find("VISCA-IP:192.168.1.40:52381"), staged.cameras.end());
    EXPECT_EQ(staged.cameras.find("ONVIF:192.168.1.41:80"), staged.cameras.end());
}

TEST(ObsbotAbsent, APluggedInCameraIsNotReplaced) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto live = std::make_shared<StandIn>(kSerial);
    live->setAlias("old");
    live->addPreset("old", Preset("old", Ptz{9.0f, 9.0f, 9}));
    mgr.addCamera(live);

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    const TempPtzc file(std::filesystem::temp_directory_path() / "ptz-obsbot-absent-live.ptzc");
    writeFile(file.path, kFile);

    commands::setup::LoadCommand command(
        [](const cameras::onvif::OnvifEndpoint&) -> std::optional<std::string> { return std::nullopt; });
    command.execute(ctx, file.path.string());

    EXPECT_EQ(mgr.findBySerial(kSerial).get(), live.get());
    EXPECT_EQ(dynamic_cast<cameras::AbsentObsbotCamera*>(live.get()), nullptr);
    EXPECT_EQ(live->getAlias(), "pulpit");
    EXPECT_EQ(live->getPresetByName("old"), nullptr);
    ASSERT_NE(live->getPresetByName("wide"), nullptr);
    EXPECT_EQ(live->getPresetByName("wide")->ptz.zoom, 40);
}

TEST(ObsbotAbsent, ShelveSaveAndRestoreLetSceneApplyMove) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto live = std::make_shared<StandIn>(kSerial);
    live->setAlias("pulpit");
    live->addPreset("wide", Preset("wide", Ptz{1.5f, -2.0f, 40}));
    mgr.addCamera(live);
    scenes.upsertBinding("sermon", kSerial, "wide");

    cameras::shelveObsbot(mgr, kSerial);
    auto shelved = mgr.findBySerial(kSerial);
    ASSERT_NE(shelved, nullptr);
    EXPECT_NE(dynamic_cast<cameras::AbsentObsbotCamera*>(shelved.get()), nullptr);
    EXPECT_NE(shelved.get(), live.get());
    EXPECT_EQ(shelved->getAlias(), "pulpit");
    EXPECT_STREQ(commands::connectionLabel(*shelved), "Offline");

    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-obsbot-absent-shelve.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, saved.path.string()));
    cameras::StagedSetup staged;
    ASSERT_EQ(store.read(saved.path.string(), staged), cameras::LoadStatus::Ok);
    ASSERT_TRUE(staged.cameras.at(kSerial).alias);
    EXPECT_EQ(*staged.cameras.at(kSerial).alias, "pulpit");
    ASSERT_EQ(staged.cameras.at(kSerial).presets.size(), 1u);
    EXPECT_EQ(staged.cameras.at(kSerial).presets[0].name, "wide");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::scene::ApplyCommand apply;
    apply.execute(ctx, "sermon");
    EXPECT_NE(err.str().find(kSerial + " is unplugged."), std::string::npos);
    EXPECT_EQ(err.str().find("camera login"), std::string::npos);
    EXPECT_FALSE(live->didMove());

    auto again = std::make_shared<StandIn>(kSerial);
    cameras::restoreObsbot(mgr, again);
    EXPECT_EQ(mgr.findBySerial(kSerial).get(), again.get());
    EXPECT_EQ(again->getAlias(), "pulpit");
    ASSERT_NE(again->getPresetByName("wide"), nullptr);
    EXPECT_EQ(again->getPresetByName("wide")->ptz.zoom, 40);

    out.str("");
    err.str("");
    apply.execute(ctx, "sermon");
    EXPECT_TRUE(again->didMove());
    EXPECT_FLOAT_EQ(again->getCurrentPtz().pan, 1.5f);
    EXPECT_FLOAT_EQ(again->getCurrentPtz().tilt, -2.0f);
    EXPECT_EQ(again->getCurrentPtz().zoom, 40);
    EXPECT_NE(out.str().find("Applied 'wide' to pulpit"), std::string::npos);
    EXPECT_EQ(err.str(), "");
}

TEST(ObsbotAbsent, DiscardRemovesThePlaceholderAndItsBindings) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto live = std::make_shared<StandIn>(kSerial);
    live->setAlias("pulpit");
    live->addPreset("wide", Preset("wide", Ptz{1.5f, -2.0f, 40}));
    mgr.addCamera(live);
    scenes.upsertBinding("sermon", kSerial, "wide");
    cameras::shelveObsbot(mgr, kSerial);

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    wireDiscard(ctx, mgr);
    commands::camera::DiscardCommand command([] { return std::optional<std::string>("y"); });
    command.execute(ctx, "pulpit");

    EXPECT_NE(out.str().find("1 preset, a binding in 1 scene. Discard pulpit? [y/N]"), std::string::npos);
    EXPECT_NE(out.str().find("Discarded " + kSerial), std::string::npos);
    EXPECT_NE(out.str().find("Scene 'sermon' is empty."), std::string::npos);
    EXPECT_EQ(err.str(), "");
    EXPECT_EQ(mgr.findBySerial(kSerial), nullptr);
    ASSERT_NE(scenes.find("sermon"), nullptr);
    EXPECT_TRUE(scenes.find("sermon")->bindings.empty());

    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-obsbot-absent-discard.ptzc");
    PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, saved.path.string()));
    cameras::StagedSetup staged;
    ASSERT_EQ(store.read(saved.path.string(), staged), cameras::LoadStatus::Ok);
    EXPECT_EQ(staged.cameras.find(kSerial), staged.cameras.end());
    EXPECT_TRUE(staged.scenes.at("sermon").bindings.empty());
}
