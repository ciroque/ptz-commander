#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "../cameras/CameraManager.h"
#include "../cameras/SceneStore.h"
#include <functional>
#include <iostream>

namespace data {
    class Context {
    public:
        cameras::CameraManager& cameraMgr;
        cameras::SceneStore& sceneStore;
        std::ostream& out;
        std::ostream& err;
        std::function<void()> onSetupChanged;

        Context(cameras::CameraManager& mgr,
                cameras::SceneStore& scenes,
                std::ostream& output = std::cout,
                std::ostream& error = std::cerr)
            : cameraMgr(mgr), sceneStore(scenes), out(output), err(error) {}

        void notifySetupChanged() {
            if (onSetupChanged) {
                onSetupChanged();
            }
        }
    };
}

#endif // DATA_CONTEXT_H