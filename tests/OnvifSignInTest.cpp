#include <gtest/gtest.h>

#include "CameraManager.h"
#include "OnvifCamera.h"
#include "OnvifCameraAdapter.h"
#include "SceneStore.h"
#include "ConnectionLabel.h"
#include "camera/LoginCommand.h"
#include "setup/LoadCommand.h"

#include "../data/Context.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
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
using cameras::onvif::OnvifError;

namespace {

    class ScriptedOnvifCamera : public cameras::onvif::OnvifCamera {
    public:
        explicit ScriptedOnvifCamera(OnvifEndpoint endpoint) : OnvifCamera(std::move(endpoint)) {}

        void setProbeResult(bool ok, std::string errorText = kAuthRejected) {
            probeOk_ = ok;
            probeError_ = std::move(errorText);
        }

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

        void markNoAnswer() {
            connected_ = false;
            lastKind_ = OnvifError::NoAnswer;
        }

        bool probe(std::string& error) override {
            if (!probeOk_) {
                error = probeError_;
                lastKind_ = OnvifError::AuthRejected;
                connected_ = false;
                return false;
            }
            lastKind_ = OnvifError::None;
            connected_ = true;
            return true;
        }

    private:
        bool probeOk_ = false;
        std::string probeError_ = kAuthRejected;
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
    EXPECT_EQ(camera.connectionState(), cameras::ConnectionState::NeedsLogin);
    EXPECT_STREQ(commands::connectionLabel(camera), "Inactive");
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
    EXPECT_EQ(camera->connectionState(), cameras::ConnectionState::Connected);
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
    EXPECT_EQ(camera->lastError(), OnvifError::AuthRejected);
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

TEST(OnvifSignIn, AddInactiveKeepsThePassword) {
    CameraManager mgr;
    OnvifCameraAdapter adapter(mgr);
    const auto result
        = adapter.addInactive(OnvifEndpoint{"192.168.1.21", 80, "/onvif/device_service", "admin", "kept", false, ""});
    EXPECT_EQ(result.status, OnvifAddStatus::Added);
    ASSERT_EQ(adapter.endpoints().size(), 1u);
    EXPECT_EQ(adapter.endpoints()[0].password, "kept");
    const auto camera = mgr.findBySerial("ONVIF:192.168.1.21:80");
    ASSERT_NE(camera, nullptr);
    EXPECT_FALSE(camera->isConnected());
    EXPECT_EQ(camera->connectionState(), cameras::ConnectionState::Offline);
    EXPECT_STREQ(commands::connectionLabel(*camera), "Offline");
}

TEST(OnvifSignIn, AddInactiveWithoutAPasswordNeedsLogin) {
    CameraManager mgr;
    OnvifCameraAdapter adapter(mgr);
    const auto result
        = adapter.addInactive(OnvifEndpoint{"192.168.1.22", 80, "/onvif/device_service", "admin", "", false, ""});
    EXPECT_EQ(result.status, OnvifAddStatus::Added);
    const auto camera = mgr.findBySerial("ONVIF:192.168.1.22:80");
    ASSERT_NE(camera, nullptr);
    EXPECT_EQ(camera->connectionState(), cameras::ConnectionState::NeedsLogin);
    EXPECT_STREQ(commands::connectionLabel(*camera), "Inactive");
}

TEST(OnvifSignIn, AddInactiveAfterAuthRejectedIsInactive) {
    CameraManager mgr;
    OnvifCameraAdapter adapter(mgr);
    auto endpoint = OnvifEndpoint{"192.168.1.27", 80, "/onvif/device_service", "admin", "kept", false, ""};
    endpoint.passwordRejected = true;
    const auto result = adapter.addInactive(endpoint);
    EXPECT_EQ(result.status, OnvifAddStatus::Added);
    const auto camera
        = std::dynamic_pointer_cast<cameras::onvif::OnvifCamera>(mgr.findBySerial("ONVIF:192.168.1.27:80"));
    ASSERT_NE(camera, nullptr);
    EXPECT_EQ(camera->lastError(), OnvifError::AuthRejected);
    EXPECT_EQ(camera->connectionState(), cameras::ConnectionState::NeedsLogin);
    EXPECT_STREQ(commands::connectionLabel(*camera), "Inactive");
    EXPECT_EQ(camera->endpoint().password, "kept");
    ASSERT_EQ(adapter.endpoints().size(), 1u);
    EXPECT_FALSE(adapter.endpoints()[0].passwordRejected);
}

TEST(OnvifSignIn, NoAnswerStaysOfflineAndLoginMakesEitherYes) {
    ScriptedOnvifCamera silent(endpoint("192.168.1.28", "kept"));
    silent.markNoAnswer();
    EXPECT_EQ(silent.connectionState(), cameras::ConnectionState::Offline);
    EXPECT_STREQ(commands::connectionLabel(silent), "Offline");

    silent.setProbeResult(true);
    std::string error;
    EXPECT_TRUE(silent.signIn("right", error));
    EXPECT_EQ(silent.connectionState(), cameras::ConnectionState::Connected);
    EXPECT_STREQ(commands::connectionLabel(silent), "Yes");

    ScriptedOnvifCamera rejected(endpoint("192.168.1.29", "kept"));
    rejected.noteAuthRejected();
    EXPECT_STREQ(commands::connectionLabel(rejected), "Inactive");
    rejected.setProbeResult(true);
    EXPECT_TRUE(rejected.signIn("right", error));
    EXPECT_STREQ(commands::connectionLabel(rejected), "Yes");
}

TEST(OnvifSignIn, LoadKeepsARejectedPasswordInactive) {
    const std::string serial = "ONVIF:192.168.1.21:80";
    nlohmann::json file = nlohmann::json::object();
    file[serial] = {{"presets", {{"wide", {{"name", "wide"}, {"pan", 4.0}, {"tilt", 1.0}, {"zoom", 8}}}}}};
    file["onvif"] = nlohmann::json::array({{{"host", "192.168.1.21"},
                                            {"port", 80},
                                            {"user", "admin"},
                                            {"password", "kept-secret"},
                                            {"path", "/onvif/device_service"}}});
    const auto path = std::filesystem::temp_directory_path() / "ptz-rejected-inactive.ptzc";
    {
        std::ofstream out(path);
        ASSERT_TRUE(out.is_open());
        out << file.dump(2);
    }
    struct RemoveFile {
        std::filesystem::path path;
        ~RemoveFile() {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    } remove{path};

    CameraManager mgr;
    cameras::SceneStore scenes;
    OnvifCameraAdapter adapter(mgr);
    std::ostringstream out;
    std::ostringstream err;
    data::Context ctx(mgr, scenes, out, err);
    ctx.addOnvif = [](const OnvifEndpoint&) {
        OnvifAddResult result;
        result.status = OnvifAddStatus::Failed;
        result.authRejected = true;
        result.detail = kAuthRejected;
        return result;
    };
    ctx.addInactiveOnvif = [&](const OnvifEndpoint& ep) {
        const auto result = adapter.addInactive(ep);
        ctx.onvifEndpoints = adapter.endpoints();
        return result;
    };

    commands::setup::LoadCommand command([](const OnvifEndpoint&) -> std::optional<std::string> {
        ADD_FAILURE() << "password was requested";
        return std::nullopt;
    });
    command.execute(ctx, path.string());

    const auto camera = std::dynamic_pointer_cast<cameras::onvif::OnvifCamera>(mgr.findBySerial(serial));
    ASSERT_NE(camera, nullptr);
    EXPECT_EQ(camera->endpoint().password, "kept-secret");
    EXPECT_EQ(camera->lastError(), OnvifError::AuthRejected);
    EXPECT_STREQ(commands::connectionLabel(*camera), "Inactive");
    EXPECT_NE(camera->getPresetByName("wide"), nullptr);
    EXPECT_NE(err.str().find("rejected the stored password"), std::string::npos);
}

TEST(OnvifSignIn, RewordedAuthTextStillCountsAsRejected) {
    CameraManager mgr;
    auto camera = std::make_shared<ScriptedOnvifCamera>(endpoint("192.168.1.26", "kept"));
    camera->setProbeResult(false, "the password was not accepted");
    mgr.addCamera(camera);

    OnvifCameraAdapter adapter(mgr);
    const auto result = adapter.login(camera->getSerialNumber(), "wrong");
    EXPECT_EQ(result.status, OnvifAddStatus::Failed);
    EXPECT_TRUE(result.authRejected);
    EXPECT_EQ(result.detail, "the password was not accepted");
    EXPECT_NE(result.detail, kAuthRejected);
    EXPECT_EQ(camera->lastError(), OnvifError::AuthRejected);
    EXPECT_EQ(camera->endpoint().password, "kept");
}
