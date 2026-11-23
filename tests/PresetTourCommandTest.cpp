#include <gtest/gtest.h>

#include "CameraManager.h"
#include "TourDelay.h"
#include "preset/ApplyCommand.h"
#include "preset/TourCommand.h"
#include "scene/TourCommand.h"

#include "../data/Context.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
using cameras::Ptz;

namespace {

    class RecordingCamera : public Camera {
    public:
        explicit RecordingCamera(std::string serial) : serial_(std::move(serial)) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return true; }
        bool setPosition(float pan, float, int) override {
            pans.push_back(pan);
            return true;
        }
        bool setZoom(int, int) override { return true; }
        Ptz getCurrentPtz() const override { return {}; }

        std::vector<float> pans;

    private:
        std::string serial_;
    };

    std::shared_ptr<RecordingCamera> addCamera(CameraManager& mgr, const std::string& serial) {
        auto camera = std::make_shared<RecordingCamera>(serial);
        mgr.addCamera(camera);
        return camera;
    }

} // namespace

TEST(PresetTourCommand, AppliesEachCamerasPresetsInNameOrder) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto left = addCamera(mgr, "VISCA-IP:192.168.1.30:52381");
    left->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    left->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    auto right = addCamera(mgr, "VISCA-IP:192.168.1.31:52381");
    right->addPreset("b", Preset("b", Ptz{15.0f, 0.0f, 0}));

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::TourCommand command;
    command.execute(ctx, "* 0");

    EXPECT_EQ(left->pans, (std::vector<float>{10.0f, 20.0f}));
    EXPECT_EQ(right->pans, (std::vector<float>{15.0f}));
    const auto a = out.str().find("Applied preset 'a'");
    const auto m = out.str().find("Applied preset 'm'");
    const auto b = out.str().find("Applied preset 'b'");
    ASSERT_NE(a, std::string::npos);
    ASSERT_NE(m, std::string::npos);
    ASSERT_NE(b, std::string::npos);
    EXPECT_LT(a, m);
    EXPECT_LT(m, b);
    EXPECT_EQ(out.str().find("Waiting"), std::string::npos);
    EXPECT_TRUE(err.str().empty());
}

TEST(PresetTourCommand, ApplyStarPrintsTheTourMessage) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = addCamera(mgr, "VISCA-IP:192.168.1.30:52381");
    camera->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    camera->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::ApplyCommand command;
    command.execute(ctx, "VISCA-IP:192.168.1.30:52381 * 0");

    EXPECT_NE(out.str().find("use preset tour VISCA-IP:192.168.1.30:52381 [seconds]"), std::string::npos);
    EXPECT_EQ(camera->pans, (std::vector<float>{10.0f, 20.0f}));

    camera->pans.clear();
    out.str("");
    err.str("");
    command.execute(ctx, "VISCA-IP:192.168.1.30:52381 a");
    EXPECT_EQ(out.str().find("use preset tour"), std::string::npos);
    EXPECT_EQ(camera->pans, (std::vector<float>{10.0f}));
    EXPECT_TRUE(err.str().empty());
}

TEST(PresetTourCommand, BadDelaysRejectedByBothTours) {
    EXPECT_DOUBLE_EQ(commands::parseTourSeconds("1.5").value_or(-1.0), 1.5);
    EXPECT_FALSE(commands::parseTourSeconds("-1"));
    EXPECT_FALSE(commands::parseTourSeconds("abc"));
    EXPECT_FALSE(commands::parseTourSeconds("inf"));

    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = addCamera(mgr, "VISCA-IP:192.168.1.30:52381");
    camera->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));

    for (const char* bad : {"-1", "abc", "inf"}) {
        std::ostringstream sceneOut;
        std::ostringstream sceneErr;
        data::Context sceneCtx(mgr, scenes, sceneOut, sceneErr);
        commands::scene::TourCommand sceneTour;
        sceneTour.execute(sceneCtx, bad);
        EXPECT_NE(sceneErr.str().find("Usage: scene tour [seconds]"), std::string::npos) << bad;
        EXPECT_EQ(sceneOut.str().find("Waiting"), std::string::npos) << bad;

        std::ostringstream presetOut;
        std::ostringstream presetErr;
        data::Context presetCtx(mgr, scenes, presetOut, presetErr);
        commands::preset::TourCommand presetTour;
        presetTour.execute(presetCtx, std::string("VISCA-IP:192.168.1.30:52381 ") + bad);
        EXPECT_NE(presetErr.str().find("Usage: preset tour <id|*> [seconds]"), std::string::npos) << bad;
        EXPECT_TRUE(camera->pans.empty()) << bad;
        EXPECT_EQ(presetOut.str().find("Applied"), std::string::npos) << bad;
    }
}
