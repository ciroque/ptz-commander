#include <gtest/gtest.h>

#include "CameraManager.h"
#include "SceneStore.h"
#include "camera/AddCommand.h"
#include "camera/LoginCommand.h"

#include "../data/Context.h"

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using cameras::Camera;
using cameras::CameraManager;
using cameras::SceneStore;
using cameras::onvif::OnvifAddResult;
using cameras::onvif::OnvifAddStatus;
using cameras::onvif::OnvifEndpoint;

namespace {

    class NamedCamera : public Camera {
    public:
        NamedCamera(std::string serial, bool connected) : serial_(std::move(serial)), connected_(connected) {}

        std::string getSerialNumber() const override { return serial_; }
        std::string getName() const override { return serial_; }
        bool isConnected() const override { return connected_; }
        bool setPosition(float, float, int) override { return connected_; }
        bool setZoom(int, int) override { return connected_; }
        cameras::Ptz getCurrentPtz() const override { return {}; }

    private:
        std::string serial_;
        bool connected_ = false;
    };

    struct Prompt {
        int calls = 0;
        std::optional<std::string> answer = std::string("secret");
        std::string user;
        std::string host;

        std::optional<std::string> ask(const OnvifEndpoint& endpoint) {
            ++calls;
            user = endpoint.user;
            host = endpoint.host;
            return answer;
        }
    };

    struct Harness {
        CameraManager mgr;
        SceneStore scenes;
        std::ostringstream out;
        std::ostringstream err;
        data::Context ctx;
        Prompt prompt;
        commands::camera::LoginCommand login;
        commands::camera::AddCommand add;
        std::vector<std::string> logins;
        std::vector<OnvifEndpoint> added;

        Harness()
            : ctx(mgr, scenes, out, err), login([this](const OnvifEndpoint& ep) { return prompt.ask(ep); }),
              add([this](const OnvifEndpoint& ep) { return prompt.ask(ep); }) {
            ctx.loginOnvif = [this](const std::string& serial, const std::string& password) {
                logins.push_back(serial + "=" + password);
                OnvifAddResult result;
                result.status = OnvifAddStatus::Added;
                return result;
            };
            ctx.addOnvif = [this](const OnvifEndpoint& ep) {
                added.push_back(ep);
                OnvifAddResult result;
                result.status = OnvifAddStatus::Added;
                return result;
            };
        }

        std::shared_ptr<NamedCamera> addCamera(const std::string& serial, bool connected) {
            auto camera = std::make_shared<NamedCamera>(serial, connected);
            mgr.addCamera(camera);
            return camera;
        }
    };

    OnvifEndpoint stored(const std::string& host, const std::string& user, const std::string& password) {
        return OnvifEndpoint{host, 80, "/onvif/device_service", user, password, false, ""};
    }

} // namespace

TEST(OnvifHiddenPassword, LoginAsksOnceAndUsesTheAnswer) {
    Harness harness;
    const auto camera = harness.addCamera("ONVIF:192.168.1.21:80", false);
    harness.ctx.onvifEndpoints = {stored("192.168.1.21", "admin", "")};

    harness.login.execute(harness.ctx, camera->getSerialNumber());

    EXPECT_EQ(harness.prompt.calls, 1);
    EXPECT_EQ(harness.prompt.user, "admin");
    EXPECT_EQ(harness.prompt.host, "192.168.1.21");
    ASSERT_EQ(harness.logins.size(), 1u);
    EXPECT_EQ(harness.logins[0], camera->getSerialNumber() + "=secret");
    EXPECT_NE(harness.out.str().find("is active"), std::string::npos);
    EXPECT_FALSE(camera->isConnected());
}

TEST(OnvifHiddenPassword, BlankLoginChangesNothing) {
    Harness harness;
    const auto camera = harness.addCamera("ONVIF:192.168.1.21:80", false);
    harness.prompt.answer = std::nullopt;

    harness.login.execute(harness.ctx, camera->getSerialNumber());

    EXPECT_EQ(harness.prompt.calls, 1);
    EXPECT_TRUE(harness.logins.empty());
    EXPECT_FALSE(camera->isConnected());
    EXPECT_EQ(harness.out.str().find("is active"), std::string::npos);
    EXPECT_NE(harness.err.str().find("No password entered."), std::string::npos);
}

TEST(OnvifHiddenPassword, LoginStarStaysRejected) {
    Harness harness;
    harness.addCamera("ONVIF:192.168.1.21:80", false);

    harness.login.execute(harness.ctx, "*");

    EXPECT_EQ(harness.prompt.calls, 0);
    EXPECT_TRUE(harness.logins.empty());
    EXPECT_NE(harness.err.str().find("Usage: camera login <id> [password]"), std::string::npos);
}

TEST(OnvifHiddenPassword, CommandLinePasswordDoesNotAsk) {
    Harness harness;
    const auto camera = harness.addCamera("ONVIF:192.168.1.21:80", false);

    harness.login.execute(harness.ctx, camera->getSerialNumber() + " secret-arg");

    EXPECT_EQ(harness.prompt.calls, 0);
    ASSERT_EQ(harness.logins.size(), 1u);
    EXPECT_EQ(harness.logins[0], camera->getSerialNumber() + "=secret-arg");
}

TEST(OnvifHiddenPassword, StoredPasswordIsLeftAlone) {
    Harness harness;
    const auto camera = harness.addCamera("ONVIF:192.168.1.21:80", true);
    harness.ctx.onvifEndpoints = {stored("192.168.1.21", "admin", "kept")};

    harness.login.execute(harness.ctx, camera->getSerialNumber());

    EXPECT_EQ(harness.prompt.calls, 0);
    EXPECT_TRUE(harness.logins.empty());
    EXPECT_TRUE(camera->isConnected());
    EXPECT_NE(harness.err.str().find("already has a password"), std::string::npos);
}

TEST(OnvifHiddenPassword, LoginOfAViscaCameraDoesNotAsk) {
    Harness harness;
    const auto camera = harness.addCamera("VISCA-IP:192.168.1.30:52381", true);

    harness.login.execute(harness.ctx, camera->getSerialNumber());

    EXPECT_EQ(harness.prompt.calls, 0);
    EXPECT_TRUE(harness.logins.empty());
    EXPECT_NE(harness.err.str().find("is not an ONVIF camera"), std::string::npos);
    EXPECT_TRUE(camera->isConnected());
}

TEST(OnvifHiddenPassword, AddOnvifAsksOnce) {
    Harness harness;

    harness.add.execute(harness.ctx, "onvif 192.168.1.21 admin");

    EXPECT_EQ(harness.prompt.calls, 1);
    EXPECT_EQ(harness.prompt.user, "admin");
    EXPECT_EQ(harness.prompt.host, "192.168.1.21");
    ASSERT_EQ(harness.added.size(), 1u);
    EXPECT_EQ(harness.added[0].password, "secret");
    EXPECT_EQ(harness.added[0].port, 80);
    EXPECT_NE(harness.out.str().find("Added "), std::string::npos);
}

TEST(OnvifHiddenPassword, AddOnvifTreatsANumericWordAsThePassword) {
    Harness harness;

    harness.add.execute(harness.ctx, "onvif 192.168.1.21 admin 8080");

    EXPECT_EQ(harness.prompt.calls, 0);
    ASSERT_EQ(harness.added.size(), 1u);
    EXPECT_EQ(harness.added[0].password, "8080");
    EXPECT_EQ(harness.added[0].port, 80);
}

TEST(OnvifHiddenPassword, AddOnvifPortFollowsThePassword) {
    Harness harness;

    harness.add.execute(harness.ctx, "onvif 192.168.1.21 admin secret 8080");

    EXPECT_EQ(harness.prompt.calls, 0);
    ASSERT_EQ(harness.added.size(), 1u);
    EXPECT_EQ(harness.added[0].password, "secret");
    EXPECT_EQ(harness.added[0].port, 8080);
}

TEST(OnvifHiddenPassword, BlankAddOnvifAddsNothing) {
    Harness harness;
    harness.prompt.answer = std::nullopt;

    harness.add.execute(harness.ctx, "onvif 192.168.1.21 admin");

    EXPECT_EQ(harness.prompt.calls, 1);
    EXPECT_TRUE(harness.added.empty());
    EXPECT_NE(harness.err.str().find("No password entered."), std::string::npos);
}

TEST(OnvifHiddenPassword, AddScannedAsksOnce) {
    Harness harness;
    harness.ctx.lastScan = std::vector<data::Context::ScannedCamera>{stored("192.168.1.30", "", "")};

    harness.add.execute(harness.ctx, "scanned 1 operator");

    EXPECT_EQ(harness.prompt.calls, 1);
    EXPECT_EQ(harness.prompt.user, "operator");
    EXPECT_EQ(harness.prompt.host, "192.168.1.30");
    ASSERT_EQ(harness.added.size(), 1u);
    EXPECT_EQ(harness.added[0].user, "operator");
    EXPECT_EQ(harness.added[0].password, "secret");
    EXPECT_EQ(harness.added[0].host, "192.168.1.30");
}

TEST(OnvifHiddenPassword, AddScannedPasswordDoesNotAsk) {
    Harness harness;
    harness.ctx.lastScan = std::vector<data::Context::ScannedCamera>{stored("192.168.1.30", "", "")};

    harness.add.execute(harness.ctx, "scanned 1 operator from-the-line");

    EXPECT_EQ(harness.prompt.calls, 0);
    ASSERT_EQ(harness.added.size(), 1u);
    EXPECT_EQ(harness.added[0].password, "from-the-line");
}

TEST(OnvifHiddenPassword, BlankAddScannedAddsNothing) {
    Harness harness;
    harness.prompt.answer = std::nullopt;
    harness.ctx.lastScan = std::vector<data::Context::ScannedCamera>{stored("192.168.1.30", "", "")};

    harness.add.execute(harness.ctx, "scanned 1 operator");

    EXPECT_EQ(harness.prompt.calls, 1);
    EXPECT_TRUE(harness.added.empty());
    EXPECT_NE(harness.err.str().find("No password entered."), std::string::npos);
}
