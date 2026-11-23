#include <gtest/gtest.h>

#include "CameraManager.h"
#include "OnvifCameraAdapter.h"
#include "PresetStore.h"
#include "SceneStore.h"
#include "ViscaCameraAdapter.h"
#include "camera/DiscardCommand.h"

#include "../data/Context.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using cameras::Camera;
using cameras::CameraManager;
using cameras::Preset;
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

    class ShelfCamera : public Camera {
    public:
        explicit ShelfCamera(std::string serial) : serial_(std::move(serial)) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return false; }
        bool setPosition(float, float, int) override { return false; }
        bool setZoom(int, int) override { return false; }
        Ptz getCurrentPtz() const override { return {}; }

    private:
        std::string serial_;
    };

    void wireDiscard(data::Context& ctx, CameraManager& mgr, cameras::visca::ViscaCameraAdapter& visca,
                     cameras::onvif::OnvifCameraAdapter& onvif) {
        ctx.discardCamera = [&](const std::string& serial) {
            if (!mgr.findBySerial(serial)) {
                return data::Context::DiscardCameraStatus::NotFound;
            }
            visca.discard(serial);
            onvif.discard(serial);
            if (mgr.findBySerial(serial)) {
                mgr.removeCamera(serial);
            }
            ctx.viscaIpEndpoints = visca.ipEndpoints();
            ctx.viscaSerialEndpoints = visca.serialEndpoints();
            ctx.onvifEndpoints = onvif.endpoints();
            return data::Context::DiscardCameraStatus::Removed;
        };
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path);
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

} // namespace

TEST(CameraDiscard, RemovesAllThreeRecordsFromTheNextSave) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    cameras::visca::ViscaCameraAdapter visca(mgr);
    cameras::onvif::OnvifCameraAdapter onvif(mgr);

    auto ip = std::make_shared<ShelfCamera>("VISCA-IP:192.168.1.40:52381");
    ip->setAlias("pulpit");
    ip->addPreset("from-ip", Preset("from-ip", Ptz{1.0f, 2.0f, 3}));
    mgr.addCamera(ip);
    cameras::visca::ViscaIpEndpoint ipEndpoint;
    ipEndpoint.host = "192.168.1.40";
    ipEndpoint.port = 52381;
    ASSERT_TRUE(visca.keepIp(ipEndpoint));

    auto serialCam = std::make_shared<ShelfCamera>("VISCA:COM7:1");
    serialCam->addPreset("from-serial", Preset("from-serial", Ptz{4.0f, 5.0f, 6}));
    mgr.addCamera(serialCam);
    cameras::visca::ViscaSerialEndpoint serialEndpoint;
    serialEndpoint.port = "COM7";
    serialEndpoint.address = 1;
    ASSERT_TRUE(visca.keepSerial(serialEndpoint));

    cameras::onvif::OnvifEndpoint onvifEndpoint;
    onvifEndpoint.host = "192.168.1.41";
    onvifEndpoint.port = 80;
    onvifEndpoint.user = "admin";
    onvifEndpoint.password = "secret";
    ASSERT_EQ(onvif.addInactive(onvifEndpoint).status, cameras::onvif::OnvifAddStatus::Added);
    auto onvifCam = mgr.findBySerial("ONVIF:192.168.1.41:80");
    ASSERT_NE(onvifCam, nullptr);
    onvifCam->addPreset("from-onvif", Preset("from-onvif", Ptz{7.0f, 8.0f, 9}));

    auto kept = std::make_shared<ShelfCamera>("KEPT:1");
    kept->addPreset("kept-wide", Preset("kept-wide", Ptz{10.0f, 11.0f, 12}));
    mgr.addCamera(kept);

    scenes.upsertBinding("sermon", ip->getSerialNumber(), "from-ip");
    scenes.upsertBinding("sermon", serialCam->getSerialNumber(), "from-serial");
    scenes.upsertBinding("sermon", "ONVIF:192.168.1.41:80", "from-onvif");
    scenes.upsertBinding("sermon", "KEPT:1", "kept-wide");
    scenes.upsertBinding("only-onvif", "ONVIF:192.168.1.41:80", "from-onvif");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    int notifies = 0;
    ctx.onSetupChanged = [&] { ++notifies; };
    wireDiscard(ctx, mgr, visca, onvif);

    commands::camera::DiscardCommand command;
    command.execute(ctx, "pulpit");
    command.execute(ctx, "VISCA:COM7:1");
    command.execute(ctx, "ONVIF:192.168.1.41:80");

    EXPECT_EQ(err.str(), "");
    EXPECT_NE(out.str().find("Discarded VISCA-IP:192.168.1.40:52381"), std::string::npos);
    EXPECT_NE(out.str().find("Discarded VISCA:COM7:1"), std::string::npos);
    EXPECT_NE(out.str().find("Discarded ONVIF:192.168.1.41:80"), std::string::npos);
    EXPECT_NE(out.str().find("Scene 'only-onvif' is empty."), std::string::npos);
    EXPECT_EQ(notifies, 3);

    EXPECT_EQ(mgr.findById("pulpit"), nullptr);
    EXPECT_EQ(mgr.findBySerial("VISCA:COM7:1"), nullptr);
    EXPECT_EQ(mgr.findBySerial("ONVIF:192.168.1.41:80"), nullptr);
    EXPECT_NE(mgr.findBySerial("KEPT:1"), nullptr);
    EXPECT_TRUE(visca.ipEndpoints().empty());
    EXPECT_TRUE(visca.serialEndpoints().empty());
    EXPECT_TRUE(onvif.endpoints().empty());
    EXPECT_TRUE(ctx.viscaIpEndpoints.empty());
    EXPECT_TRUE(ctx.viscaSerialEndpoints.empty());
    EXPECT_TRUE(ctx.onvifEndpoints.empty());

    const auto* sermon = scenes.find("sermon");
    ASSERT_NE(sermon, nullptr);
    ASSERT_EQ(sermon->bindings.size(), 1u);
    EXPECT_EQ(sermon->bindings.at("KEPT:1"), "kept-wide");
    const auto* emptied = scenes.find("only-onvif");
    ASSERT_NE(emptied, nullptr);
    EXPECT_TRUE(emptied->bindings.empty());

    const TempPtzc saved(std::filesystem::temp_directory_path() / "ptz-camera-discard.ptzc");
    cameras::PresetStore store;
    ASSERT_TRUE(store.save(mgr, scenes, ctx.viscaIpEndpoints, ctx.viscaSerialEndpoints, ctx.onvifEndpoints,
                           saved.path.string()));
    const auto text = readFile(saved.path);
    EXPECT_EQ(text.find("192.168.1.40"), std::string::npos);
    EXPECT_EQ(text.find("192.168.1.41"), std::string::npos);
    EXPECT_EQ(text.find("VISCA:COM7:1"), std::string::npos);
    EXPECT_EQ(text.find("from-ip"), std::string::npos);
    EXPECT_EQ(text.find("from-serial"), std::string::npos);
    EXPECT_EQ(text.find("from-onvif"), std::string::npos);
    EXPECT_NE(text.find("kept-wide"), std::string::npos);
    EXPECT_NE(text.find("only-onvif"), std::string::npos);
    EXPECT_EQ(text.find("\"onvif\""), std::string::npos);
    EXPECT_EQ(text.find("visca_ip"), std::string::npos);
    EXPECT_EQ(text.find("visca_serial"), std::string::npos);
}

TEST(CameraDiscard, RefusedDiscardLeavesTheCameraAndBindings) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = std::make_shared<ShelfCamera>("OBSBOTSN");
    mgr.addCamera(camera);
    scenes.upsertBinding("sermon", "OBSBOTSN", "wide");

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    int notifies = 0;
    ctx.onSetupChanged = [&] { ++notifies; };
    ctx.discardCamera = [](const std::string&) { return data::Context::DiscardCameraStatus::Obsbot; };

    commands::camera::DiscardCommand command;
    command.execute(ctx, "OBSBOTSN");

    EXPECT_NE(err.str().find("Unplug it first."), std::string::npos);
    EXPECT_EQ(out.str().find("Discarded"), std::string::npos);
    EXPECT_NE(mgr.findBySerial("OBSBOTSN"), nullptr);
    const auto* sermon = scenes.find("sermon");
    ASSERT_NE(sermon, nullptr);
    EXPECT_EQ(sermon->bindings.at("OBSBOTSN"), "wide");
    EXPECT_EQ(notifies, 0);
}

TEST(CameraDiscard, UnknownIdAndStarAreRefused) {
    CameraManager mgr;
    cameras::SceneStore scenes;
    auto camera = std::make_shared<ShelfCamera>("KEPT:1");
    mgr.addCamera(camera);

    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    ctx.discardCamera = [](const std::string&) {
        ADD_FAILURE() << "discard was called";
        return data::Context::DiscardCameraStatus::Removed;
    };

    commands::camera::DiscardCommand command;
    command.execute(ctx, "missing");
    command.execute(ctx, "*");
    command.execute(ctx, "a b");

    EXPECT_NE(err.str().find("Camera not found: missing"), std::string::npos);
    EXPECT_NE(err.str().find("camera discard does not accept '*'"), std::string::npos);
    EXPECT_NE(err.str().find("Usage: camera discard <id>"), std::string::npos);
    EXPECT_NE(mgr.findBySerial("KEPT:1"), nullptr);
    EXPECT_EQ(out.str().find("Discarded"), std::string::npos);
}
