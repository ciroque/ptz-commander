#include "TourCommand.h"
#include "ApplyCommand.h"
#include "../Arguments.h"
#include "../TourDelay.h"
#include "../TourRunner.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace commands::scene {

    void TourCommand::execute(data::Context& ctx, const std::string& args) {
        double delaySec = commands::kDefaultTourDelaySec;
        auto tokens = commands::splitArgs(args);
        if (tokens.size() > 1) {
            ctx.err << "Usage: scene tour [seconds]" << std::endl;
            return;
        }
        if (tokens.size() == 1) {
            const auto parsed = commands::parseTourSeconds(tokens[0]);
            if (!parsed) {
                ctx.err << "Usage: scene tour [seconds]" << std::endl;
                return;
            }
            delaySec = *parsed;
        }

        const auto& scenes = ctx.sceneStore.all();
        if (scenes.empty()) {
            ctx.err << "No scenes found." << std::endl;
            return;
        }

        // No runner: the delay-0 tests stay on this thread.
        if (!ctx.tour) {
            ApplyCommand apply;
            bool first = true;
            for (const auto& entry : scenes) {
                if (!first && delaySec > 0.0) {
                    ctx.out << "Waiting " << delaySec << "s..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::duration<double>(delaySec));
                }
                first = false;
                apply.execute(ctx, entry.first);
            }
            return;
        }

        std::vector<commands::TourRunner::Step> steps;
        steps.reserve(scenes.size());
        for (const auto& entry : scenes) {
            const std::string name = entry.first;
            steps.push_back([name](data::Context& stepCtx) { ApplyCommand{}.applyNamed(stepCtx, name); });
        }

        const auto delay
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(delaySec));
        ctx.tour->start(
            "scene tour", std::move(steps), delay,
            [delaySec](data::Context& stepCtx) { stepCtx.out << "Waiting " << delaySec << "s..." << std::endl; }, {},
            ctx);
    }

} // namespace commands::scene
