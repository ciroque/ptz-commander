#include "TourCommand.h"

#include "Arguments.h"
#include "TourRunner.h"

#include <iostream>

namespace commands {

    void TourCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            const std::string name = ctx.tour ? ctx.tour->running() : std::string{};
            if (name.empty()) {
                ctx.out << "No tour is running." << std::endl;
            } else {
                ctx.out << "Running " << name << std::endl;
            }
            return;
        }
        if (tokens.size() == 1 && tokens[0] == "stop") {
            if (ctx.tour && ctx.tour->stop()) {
                ctx.out << "Tour stopped." << std::endl;
            } else {
                ctx.out << "No tour is running." << std::endl;
            }
            return;
        }
        ctx.err << "Usage: tour [stop]" << std::endl;
    }

} // namespace commands
