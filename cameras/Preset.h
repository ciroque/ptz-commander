#ifndef CAMERAS_PRESET_H
#define CAMERAS_PRESET_H

#include <string>

namespace cameras {
    struct Ptz {
        float pan;
        float tilt;
        int zoom;
    };

    struct Preset {
        explicit Preset() : ptz{ 0.0f, 0.0f, 0 }, name("") {} 
		Preset(const std::string& name, const Ptz& ptz) : ptz(ptz), name(name) {}
        Ptz ptz;
        std::string name;
    };
}

#endif // CAMERAS_PRESET_H