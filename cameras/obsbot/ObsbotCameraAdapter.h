#ifndef CAMERAS_OBSBOT_OBSBOT_CAMERA_ADAPTER_H
#define CAMERAS_OBSBOT_OBSBOT_CAMERA_ADAPTER_H

#include "../CameraManager.h"

#include "../obsbot/comm.hpp"
#include "../obsbot/dev.hpp"
#include "../obsbot/devs.hpp"

#include <thread>
#include <atomic>

namespace cameras::obsbot {
    class ObsbotCameraAdapter {
    public:
        ObsbotCameraAdapter(CameraManager& manager);
        ~ObsbotCameraAdapter();

        void start();
        void stop();

    private:
        Devices& devices_;  // OBSBOT SDK singleton
        CameraManager& manager_;  // Reference to generic manager
        std::thread watchThread_;
        std::atomic<bool> running_;

        static void onDeviceChanged(std::string devSn, bool connected, void* param);
        void findDevices();
    };
}

#endif // CAMERAS_OBSBOT_OBSBOT_CAMERA_ADAPTER_H