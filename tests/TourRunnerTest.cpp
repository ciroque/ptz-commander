#include <gtest/gtest.h>

#include "TourCommand.h"
#include "TourRunner.h"
#include "preset/ApplyCommand.h"
#include "preset/TourCommand.h"
#include "scene/ApplyCommand.h"
#include "scene/TourCommand.h"

#include "../data/Context.h"

#include <memory>
#include <mutex>
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

    // runner is last so it joins before the context and the cameras go away.
    struct Harness {
        std::mutex commandLock;
        CameraManager mgr;
        cameras::SceneStore scenes;
        std::ostringstream out;
        std::ostringstream err;
        data::Context ctx;
        commands::TourRunner runner;

        Harness() : ctx(mgr, scenes, out, err), runner(commandLock) {
            runner.setManualWait(true);
            ctx.tour = &runner;
        }

        std::shared_ptr<RecordingCamera> add(const std::string& serial) {
            auto camera = std::make_shared<RecordingCamera>(serial);
            mgr.addCamera(camera);
            return camera;
        }
    };

} // namespace

TEST(TourRunner, StopEndsTheTourInsideTheWait) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_NE(h.out.str().find("Waiting 7s..."), std::string::npos);

    commands::TourCommand stop;
    stop.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_NE(h.out.str().find("Tour stopped."), std::string::npos);
    EXPECT_TRUE(h.runner.running().empty());
}

TEST(TourRunner, SecondTourStopsTheFirst) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    auto lectern = h.add("lectern");
    lectern->addPreset("wide", Preset("wide", Ptz{1.0f, 0.0f, 0}));
    lectern->addPreset("tight", Preset("tight", Ptz{2.0f, 0.0f, 0}));
    h.scenes.upsertBinding("alpha", "lectern", "wide");
    h.scenes.upsertBinding("beta", "lectern", "tight");

    commands::preset::TourCommand presetTour;
    presetTour.execute(h.ctx, "pulpit");
    h.runner.waitUntilWaiting();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_TRUE(lectern->pans.empty());

    commands::scene::TourCommand sceneTour;
    sceneTour.execute(h.ctx, "7");
    h.runner.waitUntilWaiting();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_EQ(lectern->pans, (std::vector<float>{1.0f}));
    EXPECT_NE(h.out.str().find("Applied 'wide' to lectern"), std::string::npos);

    commands::TourCommand stop;
    stop.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_EQ(lectern->pans, (std::vector<float>{1.0f}));
    EXPECT_EQ(h.out.str().find("Applied 'tight'"), std::string::npos);
}

TEST(TourRunner, MoveLockCanBeTakenWhileTheTourWaits) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    auto lectern = h.add("lectern");
    lectern->addPreset("shot", Preset("shot", Ptz{99.0f, 0.0f, 0}));
    h.scenes.upsertBinding("cue", "lectern", "shot");

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();

    ASSERT_TRUE(h.commandLock.try_lock());
    {
        struct Release {
            std::mutex& mutex;
            ~Release() { mutex.unlock(); }
        } release{h.commandLock};
        commands::scene::ApplyCommand apply;
        apply.execute(h.ctx, "cue");
    }
    h.runner.waitUntilIdle();

    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_EQ(lectern->pans, (std::vector<float>{99.0f}));
    EXPECT_TRUE(h.runner.running().empty());
}

TEST(TourRunner, StatusNamesTheRunningTour) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();

    commands::TourCommand status;
    status.execute(h.ctx, "");
    EXPECT_NE(h.out.str().find("Running preset tour pulpit"), std::string::npos);

    status.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    h.out.str("");
    h.out.clear();
    status.execute(h.ctx, "");
    EXPECT_NE(h.out.str().find("No tour is running."), std::string::npos);
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
}

TEST(TourRunner, AdvanceRunsTheNextStep) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    pulpit->addPreset("z", Preset("z", Ptz{30.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));

    h.runner.advance();
    h.runner.waitUntilWaiting();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f, 20.0f}));

    commands::TourCommand stop;
    stop.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f, 20.0f}));
}

TEST(TourRunner, DelayZeroFinishesWithoutWaiting) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 0");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f, 20.0f}));
    EXPECT_EQ(h.out.str().find("Waiting"), std::string::npos);
    EXPECT_TRUE(h.err.str().empty());
}

TEST(TourRunner, ApplyStarStopsInsideTheWait) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));

    commands::preset::ApplyCommand apply;
    apply.execute(h.ctx, "pulpit * 2000");
    h.runner.waitUntilWaiting();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_NE(h.out.str().find("use preset tour pulpit [seconds]"), std::string::npos);
    EXPECT_EQ(h.out.str().find("Waiting"), std::string::npos);
    EXPECT_EQ(h.runner.running(), "preset apply pulpit *");

    commands::TourCommand stop;
    stop.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_EQ(h.out.str().find("Applied presets to pulpit"), std::string::npos);
}

TEST(TourRunner, ApplyStarDelayZeroPrintsTheSummaryAfterTheMoves) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));

    commands::preset::ApplyCommand apply;
    apply.execute(h.ctx, "pulpit * 0");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f, 20.0f}));
    const auto text = h.out.str();
    const auto use = text.find("use preset tour pulpit [seconds]");
    const auto a = text.find("Applied preset 'a'");
    const auto m = text.find("Applied preset 'm'");
    const auto summary = text.find("Applied presets to pulpit");
    ASSERT_NE(use, std::string::npos);
    ASSERT_NE(a, std::string::npos);
    ASSERT_NE(m, std::string::npos);
    ASSERT_NE(summary, std::string::npos);
    EXPECT_LT(use, a);
    EXPECT_LT(a, m);
    EXPECT_LT(m, summary);
    EXPECT_EQ(text.find("Waiting"), std::string::npos);
    EXPECT_TRUE(h.err.str().empty());
}

TEST(TourRunner, NamedPresetApplyStopsTheTour) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    pulpit->addPreset("z", Preset("z", Ptz{30.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();

    commands::preset::ApplyCommand apply;
    apply.execute(h.ctx, "pulpit z");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f, 30.0f}));
    EXPECT_TRUE(h.runner.running().empty());
}

TEST(TourRunner, BadTourAndStarApplyDoNotStop) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();

    commands::scene::TourCommand sceneTour;
    sceneTour.execute(h.ctx, "-1");
    commands::scene::ApplyCommand sceneApply;
    sceneApply.execute(h.ctx, "*");
    commands::preset::ApplyCommand presetApply;
    presetApply.execute(h.ctx, "pulpit");
    commands::TourCommand root;
    root.execute(h.ctx, "stop please");

    EXPECT_EQ(h.runner.running(), "preset tour pulpit");
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_NE(h.err.str().find("Usage: scene tour [seconds]"), std::string::npos);
    EXPECT_NE(h.out.str().find("use scene tour"), std::string::npos);
    EXPECT_NE(h.err.str().find("Usage: preset apply <id|*> <name>"), std::string::npos);
    EXPECT_NE(h.err.str().find("Usage: tour [stop]"), std::string::npos);

    root.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
}

TEST(TourRunner, EmptyPresetTourDoesNotStop) {
    Harness h;
    auto pulpit = h.add("pulpit");
    pulpit->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    pulpit->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    h.add("empty");

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "pulpit 7");
    h.runner.waitUntilWaiting();

    tour.execute(h.ctx, "empty");
    EXPECT_EQ(h.runner.running(), "preset tour pulpit");
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
    EXPECT_NE(h.err.str().find("No presets found for empty"), std::string::npos);

    commands::TourCommand stop;
    stop.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_EQ(pulpit->pans, (std::vector<float>{10.0f}));
}

TEST(TourRunner, WaitSitsBetweenCameras) {
    Harness h;
    auto left = h.add("left");
    left->addPreset("a", Preset("a", Ptz{10.0f, 0.0f, 0}));
    left->addPreset("m", Preset("m", Ptz{20.0f, 0.0f, 0}));
    auto right = h.add("right");
    right->addPreset("b", Preset("b", Ptz{15.0f, 0.0f, 0}));

    commands::preset::TourCommand tour;
    tour.execute(h.ctx, "* 7");
    h.runner.waitUntilWaiting();
    EXPECT_EQ(left->pans, (std::vector<float>{10.0f}));
    EXPECT_TRUE(right->pans.empty());

    h.runner.advance();
    h.runner.waitUntilWaiting();
    EXPECT_EQ(left->pans, (std::vector<float>{10.0f, 20.0f}));
    EXPECT_TRUE(right->pans.empty());

    commands::TourCommand stop;
    stop.execute(h.ctx, "stop");
    h.runner.waitUntilIdle();
    EXPECT_TRUE(right->pans.empty());
}

TEST(TourRunner, StopWhenNothingIsRunning) {
    Harness h;
    commands::TourCommand tour;
    tour.execute(h.ctx, "");
    EXPECT_NE(h.out.str().find("No tour is running."), std::string::npos);
    h.out.str("");
    h.out.clear();
    tour.execute(h.ctx, "stop");
    EXPECT_NE(h.out.str().find("No tour is running."), std::string::npos);
    EXPECT_EQ(h.out.str().find("Tour stopped."), std::string::npos);
}
