#include <gtest/gtest.h>

#include "CameraManager.h"
#include "OnvifCamera.h"
#include "OnvifCameraAdapter.h"
#include "SceneStore.h"
#include "camera/LoginCommand.h"

#include "../data/Context.h"

#include <memory>
#include <sstream>
#include <string>

using cameras::CameraManager;
using cameras::Ptz;
using cameras::SceneStore;
using cameras::onvif::kAuthRejected;
using cameras::onvif::OnvifAddResult;
using cameras::onvif::OnvifAddStatus;
using cameras::onvif::OnvifCameraAdapter;
using cameras::onvif::OnvifEndpoint;

namespace {

    class ScriptedOnvifCamera : public cameras::onvif::OnvifCamera {
    public:
        explicit ScriptedOnvifCamera(OnvifEndpoint endpoint) : OnvifCamera(std::move(endpoint)) {}

        void setProbeResult(bool ok) { probeOk_ = ok; }

        void markConnected(Ptz ptz) {
            connected_ = true;
            haveRaw_ = true;
            lastPtz_ = ptz;
            lastRaw_.panX = 0.42;
            lastRaw_.tiltY = -0.1;
            lastRaw_.zoomX = 0.3;
        }

        bool haveRaw() const { return haveRaw_; }
        Ptz savedPtz() const { return lastPtz_; }
        double savedPanX() const { return lastRaw_.panX; }

        bool probe(std::string& error) override {
            if (!probeOk_) {
                error = kAuthRejected;
                connected_ = false;
                return false;
            }
            connected_ = true;
            return true;
        }

    private:
        bool probeOk_ = false;
    };

    OnvifEndpoint endpoint(const std::string& host, const std::string& password) {
        return OnvifEndpoint{host, 80, "/onvif/device_service", "admin", password, false, ""};
    }

    OnvifAddResult rejectedLogin(const std::string&, const std::string&) {
        OnvifAddResult result;
        result.status = OnvifAddStatus::Failed;
        result.authRejected = true;
        result.detail = kAuthRejected;
        return result;
    }

} // namespace

TEST(OnvifSignIn, RejectedLoginOnAnActiveCameraStaysConnected) {
    ScriptedOnvifCamera camera(endpoint("192.168.1.20", "kept"));
    camera.markConnected(Ptz{10.0f, -3.0f, 40});
    camera.setProbeResult(false);

    std::string error;
    EXPECT_FALSE(camera.signIn("wrong", error));
    EXPECT_EQ(error, kAuthRejected);
    EXPECT_TRUE(camera.isConnected());
    EXPECT_EQ(camera.endpoint().password, "kept");
    EXPECT_TRUE(camera.haveRaw());
    EXPECT_FLOAT_EQ(camera.savedPtz().pan, 10.0f);
    EXPECT_FLOAT_EQ(camera.savedPtz().tilt, -3.0f);
    EXPECT_EQ(camera.savedPtz().zoom, 40);
    EXPECT_DOUBLE_EQ(camera.savedPanX(), 0.42);
}

TEST(OnvifSignIn, RejectedLoginOnAnInactiveCameraStaysInactive) {
    ScriptedOnvifCamera camera(endpoint("192.168.1.21", "kept"));
    camera.setProbeResult(false);

    std::string error;
    EXPECT_FALSE(camera.signIn("wrong", error));
    EXPECT_EQ(error, kAuthRejected);
    EXPECT_FALSE(camera.isConnected());
    EXPECT_EQ(camera.endpoint().password, "kept");
    EXPECT_FALSE(camera.haveRaw());
}

TEST(OnvifSignIn, SuccessfulLoginStoresThePassword) {
    CameraManager mgr;
    auto camera = std::make_shared<ScriptedOnvifCamera>(endpoint("192.168.1.22", ""));
    camera->setProbeResult(true);
    mgr.addCamera(camera);

    OnvifCameraAdapter adapter(mgr);
    const auto result = adapter.login(camera->getSerialNumber(), "right");
    EXPECT_EQ(result.status, OnvifAddStatus::Added);
    ASSERT_EQ(adapter.endpoints().size(), 1u);
    EXPECT_EQ(adapter.endpoints()[0].password, "right");
    EXPECT_TRUE(camera->isConnected());
    EXPECT_EQ(camera->endpoint().password, "right");
}

TEST(OnvifSignIn, RejectedAdapterLoginOnAnActiveCameraStaysConnected) {
    CameraManager mgr;
    auto camera = std::make_shared<ScriptedOnvifCamera>(endpoint("192.168.1.23", "kept"));
    camera->markConnected(Ptz{10.0f, -3.0f, 40});
    camera->setProbeResult(false);
    mgr.addCamera(camera);

    OnvifCameraAdapter adapter(mgr);
    const auto result = adapter.login(camera->getSerialNumber(), "wrong");
    EXPECT_EQ(result.status, OnvifAddStatus::Failed);
    EXPECT_TRUE(result.authRejected);
    EXPECT_EQ(result.detail, kAuthRejected);
    EXPECT_TRUE(camera->isConnected());
    EXPECT_EQ(camera->endpoint().password, "kept");
    EXPECT_TRUE(camera->haveRaw());
    EXPECT_TRUE(adapter.endpoints().empty());
}

TEST(OnvifSignIn, LoginMessageSaysStillInactiveOnlyWhenDisconnected) {
    CameraManager mgr;
    SceneStore scenes;
    auto live = std::make_shared<ScriptedOnvifCamera>(endpoint("192.168.1.24", "kept"));
    live->markConnected(Ptz{1.0f, 2.0f, 3});
    auto idle = std::make_shared<ScriptedOnvifCamera>(endpoint("192.168.1.25", ""));
    mgr.addCamera(live);
    mgr.addCamera(idle);

    commands::camera::LoginCommand command;

    std::ostringstream liveOut;
    std::ostringstream liveErr;
    data::Context liveCtx(mgr, scenes, liveOut, liveErr);
    liveCtx.loginOnvif = rejectedLogin;
    command.execute(liveCtx, live->getSerialNumber() + " wrong");
    EXPECT_NE(liveErr.str().find("Could not log in"), std::string::npos);
    EXPECT_EQ(liveErr.str().find("is still inactive"), std::string::npos);
    EXPECT_TRUE(live->isConnected());

    std::ostringstream idleOut;
    std::ostringstream idleErr;
    data::Context idleCtx(mgr, scenes, idleOut, idleErr);
    idleCtx.loginOnvif = rejectedLogin;
    command.execute(idleCtx, idle->getSerialNumber() + " wrong");
    EXPECT_NE(idleErr.str().find("is still inactive"), std::string::npos);
    EXPECT_FALSE(idle->isConnected());
}
