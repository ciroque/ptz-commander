#include "TourRunner.h"

namespace commands {

    TourRunner::TourRunner(std::mutex& commandLock) : commandLock_(commandLock) {
        thread_ = std::thread([this] { loop(); });
    }

    TourRunner::~TourRunner() { shutdown(); }

    void TourRunner::start(std::string name, std::vector<Step> steps, std::chrono::milliseconds delay, Step between,
                           Step after, data::Context& ctx) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (shutdown_) {
                return;
            }
            ++generation_;
            // A stale wait flag or advance belongs to the tour being replaced.
            inWait_ = false;
            advance_ = false;
            Job job;
            job.generation = generation_;
            job.name = name;
            job.steps = std::move(steps);
            job.delay = delay;
            job.between = std::move(between);
            job.after = std::move(after);
            job.ctx = &ctx;
            pending_ = std::move(job);
            running_ = std::move(name);
        }
        cv_.notify_all();
    }

    bool TourRunner::stop() {
        bool was = false;
        {
            std::lock_guard<std::mutex> lock(mu_);
            was = busy_ || pending_.has_value() || !running_.empty();
            ++generation_;
            pending_.reset();
            running_.clear();
            advance_ = false;
            inWait_ = false;
        }
        cv_.notify_all();
        return was;
    }

    std::string TourRunner::running() const {
        std::lock_guard<std::mutex> lock(mu_);
        return running_;
    }

    void TourRunner::shutdown() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (shutdown_) {
                return;
            }
            shutdown_ = true;
            ++generation_;
            pending_.reset();
            running_.clear();
        }
        cv_.notify_all();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void TourRunner::setManualWait(bool enabled) {
        std::lock_guard<std::mutex> lock(mu_);
        manualWait_ = enabled;
    }

    void TourRunner::advance() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            advance_ = true;
            // Drop the current wait so the caller can block until the next one.
            inWait_ = false;
        }
        cv_.notify_all();
    }

    void TourRunner::waitUntilWaiting() {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [&] { return inWait_ || shutdown_; });
    }

    void TourRunner::waitUntilIdle() {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [&] { return !busy_ && !pending_ && running_.empty(); });
    }

    bool TourRunner::current(std::uint64_t generation) const {
        std::lock_guard<std::mutex> lock(mu_);
        return !shutdown_ && generation == generation_;
    }

    bool TourRunner::pause(const Job& job) {
        std::unique_lock<std::mutex> lock(mu_);
        if (shutdown_ || job.generation != generation_) {
            return false;
        }
        if (job.delay.count() <= 0) {
            return true;
        }
        if (manualWait_) {
            inWait_ = true;
            cv_.notify_all();
            cv_.wait(lock, [&] { return shutdown_ || generation_ != job.generation || advance_; });
            inWait_ = false;
            advance_ = false;
            return !shutdown_ && job.generation == generation_;
        }
        cv_.wait_for(lock, job.delay, [&] { return shutdown_ || generation_ != job.generation; });
        return !shutdown_ && job.generation == generation_;
    }

    void TourRunner::runJob(Job& job) {
        for (size_t i = 0; i < job.steps.size(); ++i) {
            if (!current(job.generation)) {
                return;
            }
            {
                std::lock_guard<std::mutex> step(commandLock_);
                if (!current(job.generation)) {
                    return;
                }
                job.steps[i](*job.ctx);
                const bool more = i + 1 < job.steps.size();
                if (more && job.delay.count() > 0 && job.between) {
                    job.between(*job.ctx);
                }
            }
            if (i + 1 == job.steps.size()) {
                break;
            }
            if (job.delay.count() <= 0) {
                continue;
            }
            if (!pause(job)) {
                return;
            }
        }
        if (!job.after || !current(job.generation)) {
            return;
        }
        std::lock_guard<std::mutex> step(commandLock_);
        if (!current(job.generation)) {
            return;
        }
        job.after(*job.ctx);
    }

    void TourRunner::loop() {
        while (true) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mu_);
                cv_.wait(lock, [&] { return shutdown_ || pending_.has_value(); });
                if (shutdown_ && !pending_) {
                    return;
                }
                if (!pending_) {
                    continue;
                }
                job = std::move(*pending_);
                pending_.reset();
                if (job.generation != generation_) {
                    continue;
                }
                busy_ = true;
                running_ = job.name;
            }

            runJob(job);

            {
                std::lock_guard<std::mutex> lock(mu_);
                busy_ = false;
                if (job.generation == generation_) {
                    running_.clear();
                }
            }
            cv_.notify_all();
        }
    }

} // namespace commands
