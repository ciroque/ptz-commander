#include "TourCommand.h"
#include "ApplyCommand.h"
#include "../Arguments.h"

#include <chrono>
#include <cmath>
#include <string>
#include <thread>

namespace commands::scene {

    namespace {
        constexpr double kDefaultDelaySec = 7.0;
    }

    void TourCommand::execute(data::Context& ctx, const std::string& args) {
        double delaySec = kDefaultDelaySec;
        auto tokens = commands::splitArgs(args);
        if (tokens.size() > 1) {
            ctx.err << "Usage: scene tour [seconds]" << std::endl;
            return;
        }
        if (tokens.size() == 1) {
            try {
                size_t idx = 0;
                delaySec = std::stod(tokens[0], &idx);
                if (idx != tokens[0].size() || delaySec < 0.0 || !std::isfinite(delaySec)) {
                    throw std::invalid_argument("invalid delay");
                }
            }
            catch (const std::exception&) {
                ctx.err << "Usage: scene tour [seconds]" << std::endl;
                return;
            }
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

}
