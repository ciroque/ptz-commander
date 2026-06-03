#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include "../cameras/CameraManager.h"
#include <iostream>

namespace data {
    class Context {
    public:
        cameras::CameraManager& cameraMgr;  // Reference to CameraManager from Application
        std::ostream& out;                  // For normal user output (success, tables, etc.)
        std::ostream& err;                  // For errors and usage messages

        Context(cameras::CameraManager& mgr,
                std::ostream& output = std::cout,
                std::ostream& error = std::cerr)
            : cameraMgr(mgr), out(output), err(error) {}
    };
}

#endif // DATA_CONTEXT_H