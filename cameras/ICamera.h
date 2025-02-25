#ifndef DATA_ICAMERA_H
#define DATA_ICAMERA_H

#include <string>

#include "CameraState.h"

namespace cameras {
    class ICamera {
    public:
        virtual ~ICamera() = default;

        virtual std::string getSerialNumber() const = 0;
        virtual std::string getName() const = 0;

        virtual bool setPosition(float pan, float tilt, float zoom) const = 0;
        virtual CameraState getCurrentState() const = 0;

        virtual bool isConnected() const = 0;
    };
}

#endif // DATA_ICAMERA_H