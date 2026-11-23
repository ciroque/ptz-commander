#ifndef COMMANDS_TOUR_DELAY_H
#define COMMANDS_TOUR_DELAY_H

#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>

namespace commands {

    // Shared by scene tour and preset tour. The whole token is one
    // non-negative finite number of seconds. Decimals are allowed.
    constexpr double kDefaultTourDelaySec = 7.0;

    inline std::optional<double> parseTourSeconds(const std::string& token) {
        try {
            size_t idx = 0;
            const double delay = std::stod(token, &idx);
            if (idx != token.size() || delay < 0.0 || !std::isfinite(delay)) {
                return std::nullopt;
            }
            return delay;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

} // namespace commands

#endif // COMMANDS_TOUR_DELAY_H
