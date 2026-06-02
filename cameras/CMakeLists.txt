# Camera management and Obsbot-specific implementations
target_sources(ptz_commander PRIVATE
    Camera.h
    CameraManager.cpp
    CameraManager.h
    Preset.h

    obsbot/ObsbotCamera.cpp
    obsbot/ObsbotCamera.h
    obsbot/ObsbotCameraAdapter.cpp
    obsbot/ObsbotCameraAdapter.h

    obsbot/strategies/ObsbotControlStrategy.h
    obsbot/strategies/StrategyUtils.h
    obsbot/strategies/TailAirStrategy.cpp
    obsbot/strategies/TailAirStrategy.h
    obsbot/strategies/TinyFamilyStrategy.cpp
    obsbot/strategies/TinyFamilyStrategy.h
)
