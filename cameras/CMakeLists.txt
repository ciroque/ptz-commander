# Camera management and Obsbot-specific implementations
target_sources(ptz_commander PRIVATE
    Camera.h
    CameraManager.cpp
    CameraManager.h
    Preset.h
    PresetStore.cpp
    PresetStore.h

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

    # VISCA serial camera support (absolute positioning only, auto-discovery for Keyspan USA-19H etc.)
    visca/ViscaCamera.cpp
    visca/ViscaCamera.h
    visca/ViscaCameraAdapter.cpp
    visca/ViscaCameraAdapter.h
    visca/ViscaCommands.cpp
    visca/ViscaCommands.h
    visca/ViscaDiscovery.cpp
    visca/ViscaDiscovery.h
    visca/SerialTransport.h
    visca/WindowsSerialTransport.cpp
    visca/WindowsSerialTransport.h
)
