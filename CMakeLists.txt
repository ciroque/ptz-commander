cmake_minimum_required(VERSION 3.28)
project(ptz_commander)

set(CMAKE_CXX_STANDARD 20)

add_executable(ptz_commander main.cpp)

# Modular source organization via subdirectories + target_sources
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(cameras)
add_subdirectory(commands)

# Include directories for third-party headers (nlohmann/json, obsbot SDK, spdlog)
target_include_directories(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/include/")

# Link against obsbot SDK
target_link_libraries(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.lib")

# VISCA serial discovery uses SetupAPI on Windows (VID/PID enumeration for Keyspan etc.)
if(WIN32)
    target_link_libraries(ptz_commander PRIVATE setupapi)
endif()

# Copy libdev.dll to the build directory after building (so the exe can find it at runtime)
add_custom_command(TARGET ptz_commander POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.dll"
        "$<TARGET_FILE_DIR:ptz_commander>"
    COMMENT "Copying libdev.dll to output directory"
)
