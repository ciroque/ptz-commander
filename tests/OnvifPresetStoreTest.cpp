#include <gtest/gtest.h>

#include "CameraManager.h"
#include "preset/StoreCommand.h"

#include "../data/Context.h"

#include <memory>
#include <sstream>
#include <string>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
using cameras::Ptz;

namespace {

    class ShelfCamera : public Camera {
    public:
        explicit ShelfCamera(std::string serial, bool connected, Ptz position)
            : serial_(std::move(serial)), connected_(connected), position_(position) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return connected_; }
        bool setPosition(float, float, int) override { return connected_; }
        bool setZoom(int, int) override { return connected_; }
        Ptz getCurrentPtz() const override { return position_; }

    private:
        std::string serial_;
        bool connected_ = false;
        Ptz position_{0.0f, 0.0f, 0};
    };

} // namespace

TEST(PresetStoreCommand, RefusesAnInactiveOnvifCamera) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = std::make_shared<ShelfCamera>("ONVIF:192.168.1.21:80", false, Ptz{0.0f, 0.0f, 0});
    camera->addPreset("sermon", Preset("sermon", Ptz{12.0f, -3.0f, 40}));
    mgr.addCamera(camera);

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::StoreCommand command;
    command.execute(ctx, "ONVIF:192.168.1.21:80 sermon");

    const auto* preset = camera->getPresetByName("sermon");
    ASSERT_NE(preset, nullptr);
    EXPECT_FLOAT_EQ(preset->ptz.pan, 12.0f);
    EXPECT_FLOAT_EQ(preset->ptz.tilt, -3.0f);
    EXPECT_EQ(preset->ptz.zoom, 40);
    EXPECT_NE(err.str().find("is inactive"), std::string::npos);
    EXPECT_EQ(out.str().find("Stored preset"), std::string::npos);
}

TEST(PresetStoreCommand, StarStoresTheOtherCameras) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto inactive = std::make_shared<ShelfCamera>("ONVIF:192.168.1.21:80", false, Ptz{0.0f, 0.0f, 0});
    inactive->addPreset("sermon", Preset("sermon", Ptz{12.0f, -3.0f, 40}));
    auto live = std::make_shared<ShelfCamera>("VISCA-IP:192.168.1.30:52381", true, Ptz{4.5f, 1.0f, 20});
    mgr.addCamera(inactive);
    mgr.addCamera(live);

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    commands::preset::StoreCommand command;
    command.execute(ctx, "* sermon");

    const auto* kept = inactive->getPresetByName("sermon");
    ASSERT_NE(kept, nullptr);
    EXPECT_FLOAT_EQ(kept->ptz.pan, 12.0f);
    EXPECT_EQ(kept->ptz.zoom, 40);

    const auto* stored = live->getPresetByName("sermon");
    ASSERT_NE(stored, nullptr);
    EXPECT_FLOAT_EQ(stored->ptz.pan, 4.5f);
    EXPECT_FLOAT_EQ(stored->ptz.tilt, 1.0f);
    EXPECT_EQ(stored->ptz.zoom, 20);

    EXPECT_NE(err.str().find("is inactive"), std::string::npos);
    EXPECT_NE(out.str().find("Stored preset 'sermon' for 1 camera"), std::string::npos);
}
