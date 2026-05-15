#ifndef CAMERAS_OBSBOT_OBSBOT_CAMERA_H
#define CAMERAS_OBSBOT_OBSBOT_CAMERA_H

#include "../Camera.h"
#include "../../include/obsbot/comm.hpp"
#include "../../include/obsbot/dev.hpp"
#include "../../include/obsbot/devs.hpp"
#include <memory>

namespace cameras::obsbot::strategies {
    class ObsbotControlStrategy;   // forward declaration
}

namespace cameras {

    class ObsbotCamera : public Camera {
    public:
        explicit ObsbotCamera(std::shared_ptr<Device> dev);
        ~ObsbotCamera() override;                    // declared here

        std::string getSerialNumber() const override;
        std::string getName() const override;
        bool isConnected() const override;

        bool setPosition(float pan, float tilt, int zoom) override;
        bool setZoom(int zoom, int speed) override;
        Ptz getCurrentPtz() const override;

    private:
        std::shared_ptr<Device> device_;
        std::unique_ptr<cameras::obsbot::strategies::ObsbotControlStrategy> strategy_;   // note: strategies:: not full path

        static constexpr float RollValue = 0.0f;
        static constexpr float MaxMoveSpeed = 75.0f;
    };

} // namespace cameras

#endif