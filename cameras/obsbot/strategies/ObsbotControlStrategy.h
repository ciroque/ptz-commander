#pragma once

#include "../../../include/obsbot/dev.hpp"
#include <memory>

namespace cameras::obsbot::strategies {

    class ObsbotControlStrategy {
    public:
        virtual ~ObsbotControlStrategy() = default;

        virtual bool disableAI(Device* dev) = 0;
        virtual bool moveTo(float pan, float tilt, int zoom, Device* dev) = 0;
        virtual bool setZoom(int zoom, int speed, Device* dev) = 0;

        // Future extension points
        // virtual bool goToPreset(...) = 0;
        // virtual bool getCurrentPtz(...) = 0;
    };

} // namespace cameras::obsbot::strategies