#include <gtest/gtest.h>

#include "CameraManager.h"
#include "scene/AddCommand.h"
#include "scene/StoreCommand.h"

#include "../data/Context.h"

#include <memory>
#include <sstream>
#include <string>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
using cameras::Ptz;

namespace {

    class NamedCamera : public Camera {
    public:
        explicit NamedCamera(std::string serial) : serial_(std::move(serial)) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return true; }
        bool setPosition(float, float, int) override { return true; }
        bool setZoom(int, int) override { return true; }
        Ptz getCurrentPtz() const override { return {}; }

    private:
        std::string serial_;
    };

    std::shared_ptr<NamedCamera> cameraWith(CameraManager& mgr, const std::string& serial, const std::string& preset) {
        auto camera = std::make_shared<NamedCamera>(serial);
        camera->addPreset(preset, Preset(preset, Ptz{1.0f, 2.0f, 3}));
        mgr.addCamera(camera);
        return camera;
    }

} // namespace

TEST(SceneStoreCommand, StoreCreatesThenReplacesABinding) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto wide = cameraWith(mgr, "VISCA-IP:192.168.1.30:52381", "wide");
    wide->addPreset("tight", Preset("tight", Ptz{4.0f, 5.0f, 6}));
    auto other = cameraWith(mgr, "VISCA-IP:192.168.1.31:52381", "close");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    int notices = 0;
    ctx.onSetupChanged = [&] { ++notices; };

    commands::scene::StoreCommand command;
    command.execute(ctx, "sermon VISCA-IP:192.168.1.30:52381 wide VISCA-IP:192.168.1.31:52381 close");

    const auto* scene = scenes.find("sermon");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->bindings.at(wide->getSerialNumber()), "wide");
    EXPECT_EQ(scene->bindings.at(other->getSerialNumber()), "close");
    EXPECT_NE(out.str().find("Stored 2 bindings in scene 'sermon'"), std::string::npos);
    EXPECT_EQ(notices, 1);

    out.str("");
    command.execute(ctx, "sermon VISCA-IP:192.168.1.30:52381 tight");
    scene = scenes.find("sermon");
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->bindings.at(wide->getSerialNumber()), "tight");
    EXPECT_EQ(scene->bindings.at(other->getSerialNumber()), "close");
    EXPECT_NE(out.str().find("Stored 1 binding in scene 'sermon'"), std::string::npos);
    EXPECT_EQ(out.str().find("scene add is now scene store"), std::string::npos);
    EXPECT_TRUE(err.str().empty());
    EXPECT_EQ(notices, 2);

    out.str("");
    err.str("");
    command.execute(ctx, "sermon * wide");
    EXPECT_EQ(scenes.find("sermon")->bindings.at(wide->getSerialNumber()), "tight");
    EXPECT_NE(err.str().find("does not accept '*'"), std::string::npos);
    EXPECT_EQ(out.str().find("Stored"), std::string::npos);
    EXPECT_EQ(notices, 2);
}

TEST(SceneStoreCommand, AddStillWorksAndPrintsTheRename) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = cameraWith(mgr, "VISCA-IP:192.168.1.30:52381", "wide");
    camera->addPreset("tight", Preset("tight", Ptz{4.0f, 5.0f, 6}));

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);

    commands::scene::AddCommand command;
    command.execute(ctx, "sermon VISCA-IP:192.168.1.30:52381 wide");
    EXPECT_NE(out.str().find("scene add is now scene store"), std::string::npos);
    EXPECT_NE(out.str().find("Stored 1 binding in scene 'sermon'"), std::string::npos);
    EXPECT_EQ(scenes.find("sermon")->bindings.at(camera->getSerialNumber()), "wide");

    command.execute(ctx, "sermon VISCA-IP:192.168.1.30:52381 tight");
    EXPECT_EQ(scenes.find("sermon")->bindings.at(camera->getSerialNumber()), "tight");
    EXPECT_EQ(err.str(), "");
}
