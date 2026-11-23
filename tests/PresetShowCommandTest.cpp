#include <gtest/gtest.h>

#include "CameraManager.h"
#include "preset/ShowCommand.h"

#include "../data/Context.h"

#include <iomanip>
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

    std::shared_ptr<NamedCamera> addCamera(CameraManager& mgr, const std::string& serial) {
        auto camera = std::make_shared<NamedCamera>(serial);
        mgr.addCamera(camera);
        return camera;
    }

    std::string presetRow(const std::string& name, float pan, float tilt, int zoom) {
        std::ostringstream line;
        line << std::left << std::setw(20) << name << " | " << std::setw(10) << pan << " | " << std::setw(10) << tilt
             << " | " << std::setw(10) << zoom << "\n";
        return line.str();
    }

} // namespace

TEST(PresetShowCommand, OneCamera) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto left = addCamera(mgr, "VISCA-IP:192.168.1.30:52381");
    left->addPreset("sermon", Preset("sermon", Ptz{12.5f, -3.0f, 40}));
    auto right = addCamera(mgr, "VISCA-IP:192.168.1.31:52381");
    right->addPreset("wide", Preset("wide", Ptz{1.0f, 2.0f, 3}));

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::ShowCommand command;
    command.execute(ctx, "VISCA-IP:192.168.1.30:52381");

    EXPECT_NE(out.str().find("Camera: VISCA-IP:192.168.1.30:52381"), std::string::npos);
    EXPECT_NE(out.str().find("Preset Name"), std::string::npos);
    EXPECT_NE(out.str().find(presetRow("sermon", 12.5f, -3.0f, 40)), std::string::npos);
    EXPECT_EQ(out.str().find("VISCA-IP:192.168.1.31:52381"), std::string::npos);
    EXPECT_EQ(out.str().find("wide"), std::string::npos);
    EXPECT_TRUE(err.str().empty());
}

TEST(PresetShowCommand, StarShowsEveryCamera) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto left = addCamera(mgr, "VISCA-IP:192.168.1.30:52381");
    left->addPreset("sermon", Preset("sermon", Ptz{12.5f, -3.0f, 40}));
    auto right = addCamera(mgr, "VISCA-IP:192.168.1.31:52381");
    right->addPreset("wide", Preset("wide", Ptz{1.0f, 2.0f, 3}));

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::ShowCommand command;
    command.execute(ctx, "*");

    EXPECT_NE(out.str().find("Camera: VISCA-IP:192.168.1.30:52381"), std::string::npos);
    EXPECT_NE(out.str().find("Camera: VISCA-IP:192.168.1.31:52381"), std::string::npos);
    EXPECT_NE(out.str().find(presetRow("sermon", 12.5f, -3.0f, 40)), std::string::npos);
    EXPECT_NE(out.str().find(presetRow("wide", 1.0f, 2.0f, 3)), std::string::npos);
    EXPECT_TRUE(err.str().empty());
}

TEST(PresetShowCommand, NamedPresetMissingOnOneCamera) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto left = addCamera(mgr, "VISCA-IP:192.168.1.30:52381");
    left->addPreset("sermon", Preset("sermon", Ptz{12.5f, -3.0f, 40}));
    auto right = addCamera(mgr, "VISCA-IP:192.168.1.31:52381");
    right->addPreset("wide", Preset("wide", Ptz{1.0f, 2.0f, 3}));

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::ShowCommand command;
    command.execute(ctx, "* sermon");

    EXPECT_NE(out.str().find(presetRow("sermon", 12.5f, -3.0f, 40)), std::string::npos);
    EXPECT_EQ(out.str().find("wide"), std::string::npos);
    EXPECT_EQ(out.str().find("VISCA-IP:192.168.1.31:52381"), std::string::npos);
    EXPECT_NE(err.str().find("Preset not found: sermon for VISCA-IP:192.168.1.31:52381"), std::string::npos);
}

TEST(PresetShowCommand, UnknownCameraMatchesApply) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::ShowCommand command;
    command.execute(ctx, "missing");

    EXPECT_TRUE(out.str().empty());
    EXPECT_NE(err.str().find("Camera not found: missing"), std::string::npos);
}
