#ifndef CAMERAS_SCENE_H
#define CAMERAS_SCENE_H

#include <map>
#include <string>

namespace cameras {

    struct Scene {
        std::string name;
        std::map<std::string, std::string> bindings;
    };

}

#endif // CAMERAS_SCENE_H
