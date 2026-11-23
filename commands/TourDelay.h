#ifndef COMMANDS_TOUR_DELAY_H
#define COMMANDS_TOUR_DELAY_H

#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

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

    // One optional --background, in any position. Any other -- word, or a
    // second --background, is rejected. The other tokens stay in order.
    struct TourWords {
        bool background = false;
        std::vector<std::string> words;
    };

    inline std::optional<TourWords> splitTourWords(const std::vector<std::string>& tokens) {
        TourWords out;
        for (const auto& token : tokens) {
            if (token == "--background") {
                if (out.background) {
                    return std::nullopt;
                }
                out.background = true;
                continue;
            }
            if (token.rfind("--", 0) == 0) {
                return std::nullopt;
            }
            out.words.push_back(token);
        }
        return out;
    }

} // namespace commands

#endif // COMMANDS_TOUR_DELAY_H
