// ObsbotCameraAdapter.h
#ifndef CAMERAS_OBSBOT_OBSBOT_CAMERA_ADAPTER_H
#define CAMERAS_OBSBOT_OBSBOT_CAMERA_ADAPTER_H

#include "../CameraManager.h"
#include "../../include/obsbot/comm.hpp"
#include "../../include/obsbot/dev.hpp"
#include "../../include/obsbot/devs.hpp"

#include <thread>
#include <atomic>

namespace cameras::obsbot {
    class ObsbotCameraAdapter {
    public:
        explicit ObsbotCameraAdapter(CameraManager& manager);
        ~ObsbotCameraAdapter();

        void start();
        void stop();

    private:
        Devices& devices_;
        CameraManager& manager_;
        std::atomic<bool> running_{ false };

        static void onDeviceChanged(std::string devSn, bool connected, void* param);
        void findDevices();
    };
}

#endif