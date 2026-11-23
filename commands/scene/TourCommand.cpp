#include "TourCommand.h"
#include "ApplyCommand.h"
#include "../Arguments.h"
#include "../TourDelay.h"

#include <chrono>
#include <string>
#include <thread>

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
    }

} // namespace commands::scene
