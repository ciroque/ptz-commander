#include <gtest/gtest.h>

#include "CameraManager.h"
#include "scene/ApplyCommand.h"
#include "scene/DiscardCommand.h"
#include "scene/ShowCommand.h"

#include "../data/Context.h"

#include <memory>
#include <optional>
#include <sstream>
#include <string>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
using cameras::Ptz;

namespace {

    class ShelfCamera : public Camera {
    public:
        explicit ShelfCamera(std::string serial) : serial_(std::move(serial)) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return true; }
        bool setPosition(float pan, float, int) override {
            ++moves;
            lastPan = pan;
            return true;
        }
        bool setZoom(int, int) override { return true; }
        Ptz getCurrentPtz() const override { return {}; }

        int moves = 0;
        float lastPan = 0.0f;

    private:
        std::string serial_;
    };

    std::shared_ptr<ShelfCamera> addCamera(CameraManager& mgr, const std::string& serial, const std::string& alias,
                                           const std::string& preset, float pan) {
        auto camera = std::make_shared<ShelfCamera>(serial);
        camera->setAlias(alias);
        camera->addPreset(preset, Preset(preset, Ptz{pan, 1.0f, 2}));
        mgr.addCamera(camera);
        return camera;
    }

    void storeBinding(cameras::SceneStore& scenes, const std::string& scene, const std::string& serial,
                      const std::string& preset) {
        scenes.upsertBinding(scene, serial, preset);
    }

} // namespace

TEST(SceneStarCommand, ShowStarPrintsEveryScene) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    addCamera(mgr, "VISCA-IP:192.168.1.30:52381", "Left", "wide", 4.0f);
    addCamera(mgr, "VISCA-IP:192.168.1.31:52381", "Right", "tight", 8.0f);
    storeBinding(scenes, "beta", "VISCA-IP:192.168.1.31:52381", "tight");
    storeBinding(scenes, "alpha", "VISCA-IP:192.168.1.30:52381", "wide");
    scenes.upsertBinding("gamma", "VISCA-IP:192.168.1.30:52381", "wide");
    scenes.removeBinding("gamma", "VISCA-IP:192.168.1.30:52381");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::scene::ShowCommand command;

    command.execute(ctx, "");
    EXPECT_NE(err.str().find("Usage: scene show <name|*>"), std::string::npos);

    err.str("");
    command.execute(ctx, "*");
    EXPECT_TRUE(err.str().empty());
    const auto text = out.str();
    const auto alpha = text.find("Scene: alpha");
    const auto beta = text.find("Scene: beta");
    const auto gamma = text.find("Scene: gamma");
    ASSERT_NE(alpha, std::string::npos);
    ASSERT_NE(beta, std::string::npos);
    ASSERT_NE(gamma, std::string::npos);
    EXPECT_LT(alpha, beta);
    EXPECT_LT(beta, gamma);
    EXPECT_NE(text.find("Left"), std::string::npos);
    EXPECT_NE(text.find("wide"), std::string::npos);
    EXPECT_NE(text.find("Right"), std::string::npos);
    EXPECT_NE(text.find("tight"), std::string::npos);
    EXPECT_NE(text.find("(no bindings)"), std::string::npos);

    out.str("");
    err.str("");
    command.execute(ctx, "missing");
    EXPECT_NE(err.str().find("Scene not found: missing"), std::string::npos);
    EXPECT_EQ(out.str().find("Scene:"), std::string::npos);

    scenes.clear();
    out.str("");
    err.str("");
    command.execute(ctx, "*");
    EXPECT_NE(err.str().find("No scenes found."), std::string::npos);
}

TEST(SceneStarCommand, DiscardStarConfirmedDeletesEveryScene) {
    for (const char* answer : {"y", "Y"}) {
        CameraManager mgr;
        cameras::SceneStore scenes;
        addCamera(mgr, "VISCA-IP:192.168.1.30:52381", "Left", "wide", 4.0f);
        storeBinding(scenes, "alpha", "VISCA-IP:192.168.1.30:52381", "wide");
        storeBinding(scenes, "beta", "VISCA-IP:192.168.1.30:52381", "wide");
        storeBinding(scenes, "gamma", "VISCA-IP:192.168.1.30:52381", "wide");

        std::ostringstream out;
        std::ostringstream err;
        data::Context ctx(mgr, scenes, out, err);
        int notices = 0;
        ctx.onSetupChanged = [&] { ++notices; };
        int asks = 0;
        commands::scene::DiscardCommand command([&] {
            ++asks;
            return std::optional<std::string>(answer);
        });

        command.execute(ctx, "gamma");
        EXPECT_EQ(asks, 0);
        EXPECT_EQ(scenes.find("gamma"), nullptr);
        EXPECT_NE(scenes.find("alpha"), nullptr);
        EXPECT_EQ(notices, 1);

        command.execute(ctx, "*");
        EXPECT_EQ(asks, 1);
        EXPECT_TRUE(scenes.empty());
        EXPECT_NE(out.str().find("Discard every scene? [y/N]"), std::string::npos);
        EXPECT_NE(out.str().find("Discarded 2 scenes"), std::string::npos);
        EXPECT_EQ(err.str().find("needs a console"), std::string::npos);
        EXPECT_EQ(notices, 2);
    }
}

TEST(SceneStarCommand, DiscardStarDeclinedKeepsEveryScene) {
    const std::optional<std::string> answers[]
        = {std::optional<std::string>("n"), std::optional<std::string>(""), std::optional<std::string>("yes"),
           std::optional<std::string>("N"), std::nullopt};
    for (const auto& answer : answers) {
        CameraManager mgr;
        cameras::SceneStore scenes;
        addCamera(mgr, "VISCA-IP:192.168.1.30:52381", "Left", "wide", 4.0f);
        storeBinding(scenes, "alpha", "VISCA-IP:192.168.1.30:52381", "wide");
        storeBinding(scenes, "beta", "VISCA-IP:192.168.1.30:52381", "wide");

        std::ostringstream out;
        std::ostringstream err;
        data::Context ctx(mgr, scenes, out, err);
        int notices = 0;
        ctx.onSetupChanged = [&] { ++notices; };
        commands::scene::DiscardCommand command([&] { return answer; });
        command.execute(ctx, "*");

        EXPECT_NE(scenes.find("alpha"), nullptr) << out.str() << err.str();
        EXPECT_NE(scenes.find("beta"), nullptr);
        EXPECT_EQ(notices, 0);
        EXPECT_EQ(out.str().find("Discarded"), std::string::npos);
        if (answer) {
            EXPECT_NE(out.str().find("Discard declined."), std::string::npos);
            EXPECT_EQ(err.str().find("needs a console"), std::string::npos);
        } else {
            EXPECT_NE(err.str().find("scene discard * needs a console."), std::string::npos);
            EXPECT_EQ(out.str().find("Discard declined."), std::string::npos);
        }
    }
}

TEST(SceneStarCommand, DiscardStarWithNoScenesDoesNotAsk) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::scene::DiscardCommand command([]() -> std::optional<std::string> {
        ADD_FAILURE() << "confirm was called";
        return std::string("y");
    });
    command.execute(ctx, "*");
    EXPECT_NE(err.str().find("No scenes found."), std::string::npos);
    EXPECT_EQ(out.str().find("Discard every scene?"), std::string::npos);
}

TEST(SceneStarCommand, ApplyStarPrintsTheTourMessage) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = addCamera(mgr, "VISCA-IP:192.168.1.30:52381", "Left", "wide", 4.0f);
    storeBinding(scenes, "sermon", camera->getSerialNumber(), "wide");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::scene::ApplyCommand command;
    command.execute(ctx, "*");

    EXPECT_EQ(camera->moves, 0);
    EXPECT_NE(scenes.find("sermon"), nullptr);
    EXPECT_NE(out.str().find("use scene tour"), std::string::npos);
    EXPECT_EQ(out.str().find("Applied"), std::string::npos);
    EXPECT_TRUE(err.str().empty());

    out.str("");
    command.execute(ctx, "sermon");
    EXPECT_EQ(camera->moves, 1);
    EXPECT_FLOAT_EQ(camera->lastPan, 4.0f);
    EXPECT_NE(out.str().find("Applied 'wide'"), std::string::npos);
}
