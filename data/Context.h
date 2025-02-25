#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "../cameras/CameraManager.h"

namespace data {
    class Context {
    public:
        static constexpr int noCameraSelected = -1;
        
        cameras::CameraManager& cameraMgr;  // Reference to CameraManager from Application
        
        int selectedDevice = noCameraSelected;

        Context(cameras::CameraManager& mgr)
            : cameraMgr(mgr), selectedDevice(-1) {}
    };
}

#endif // DATA_CONTEXT_H