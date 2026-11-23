#ifndef COMMANDS_TOURRUNNER_H
#define COMMANDS_TOURRUNNER_H

#include "../data/Context.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace commands {

    // One background tour. Application installs it. Without one, scene tour,
    // preset tour, and the preset apply * cycle stay inline, which is what
    // the delay-0 tests use.
    //
    // The worker takes commandLock only around one step and the lines that
    // step prints, then releases it before waiting. Stop, or a newer start,
    // wakes the wait at once. The old steps do not move again.
    class TourRunner {
    public:
        using Step = std::function<void(data::Context&)>;

        explicit TourRunner(std::mutex& commandLock);
        ~TourRunner();

        TourRunner(const TourRunner&) = delete;
        TourRunner& operator=(const TourRunner&) = delete;

        // Replaces any tour already running. Returns as soon as the worker
        // has been told. between runs under the command lock after a step
        // when another step follows and the delay is positive (the
        // "Waiting" line). after runs under the lock once the last step
        // has finished, if this tour was not replaced.
        void start(std::string name, std::vector<Step> steps, std::chrono::milliseconds delay, Step between, Step after,
                   data::Context& ctx);

        // True when a tour was running or queued.
        bool stop();

        // Empty when nothing is running.
        std::string running() const;

        // Join the worker. Application calls this before tearing cameras down.
        void shutdown();

        // Tests: the wait blocks on the condition variable until stop, start,
        // or advance, and does not use the clock.
        void setManualWait(bool enabled);
        void advance();
        void waitUntilWaiting();
        void waitUntilIdle();

    private:
        struct Job {
            std::uint64_t generation = 0;
            std::string name;
            std::vector<Step> steps;
            std::chrono::milliseconds delay{0};
            Step between;
            Step after;
            data::Context* ctx = nullptr;
        };

        void loop();
        void runJob(Job& job);
        bool current(std::uint64_t generation) const;
        bool pause(const Job& job);

        std::mutex& commandLock_;
        mutable std::mutex mu_;
        std::condition_variable cv_;
        std::thread thread_;
        bool shutdown_ = false;
        bool busy_ = false;
        bool inWait_ = false;
        bool advance_ = false;
        bool manualWait_ = false;
        std::uint64_t generation_ = 0;
        std::string running_;
        std::optional<Job> pending_;
    };

} // namespace commands

#endif // COMMANDS_TOURRUNNER_H
