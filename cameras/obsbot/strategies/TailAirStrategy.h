#pragma once
#include "ObsbotControlStrategy.h"

namespace cameras::obsbot::strategies {

    class TailAirStrategy : public ObsbotControlStrategy {
    public:
        bool disableAI(Device* dev) override;
        bool moveTo(float pan, float tilt, int zoom, Device* dev) override;
        bool setZoom(int zoom, int speed, Device* dev) override;
    };

} // namespace cameras::obsbot::strategies