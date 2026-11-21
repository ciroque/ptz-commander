include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
# The installed Visual Studio runtime is shared. Forcing gtest onto the
# static CRT mismatches the test executable and fails to link.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# cmake --install stages the WiX prefix. The release build only builds
# ptz_commander, so gtest and gmock are not there, and the MSI does not ship them.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(ptz_tests
    SplitArgsTest.cpp
    ViscaRangeTest.cpp
    ViscaIpFramingTest.cpp
    OnvifParseTest.cpp
    OnvifPasswordTest.cpp
    OnvifSetupLoadTest.cpp
    OnvifPortableSetupTest.cpp
    OnvifSignInTest.cpp
    "${CMAKE_SOURCE_DIR}/commands/camera/LoginCommand.cpp"
    "${CMAKE_SOURCE_DIR}/commands/setup/LoadCommand.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/onvif/OnvifCamera.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/onvif/OnvifCameraAdapter.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/onvif/OnvifClient.cpp"
    "${CMAKE_SOURCE_DIR}/commands/setup/ExportCommand.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/CameraManager.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/PresetStore.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/visca/ViscaCamera.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/visca/ViscaCommands.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/visca/ViscaCalibration.cpp"
    "${CMAKE_SOURCE_DIR}/cameras/onvif/OnvifPassword.cpp"
)

target_include_directories(ptz_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/commands"
    "${CMAKE_SOURCE_DIR}/cameras"
    "${CMAKE_SOURCE_DIR}/cameras/visca"
    "${CMAKE_SOURCE_DIR}/cameras/onvif"
)

target_link_libraries(ptz_tests PRIVATE
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
)
target_compile_features(ptz_tests PRIVATE cxx_std_20)
if(WIN32)
    target_link_libraries(ptz_tests PRIVATE crypt32 winhttp bcrypt)
endif()
ptz_pin_msvc_x64_libs(ptz_tests)

include(GoogleTest)
# PRE_TEST discovers cases when ctest runs, which works for the Visual Studio
# multi-config generator. Configure time would need the exe already built.
gtest_discover_tests(ptz_tests DISCOVERY_MODE PRE_TEST)
