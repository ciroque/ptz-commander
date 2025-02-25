cmake_minimum_required(VERSION 3.28)
project(ptz_commander)

set(CMAKE_CXX_STANDARD 20)

add_executable(ptz_commander main.cpp
        commands/Command.cpp
        commands/Command.h
        commands/ListCommand.cpp
        commands/ListCommand.h
        
        data/Context.h
        commands/ShowCommand.cpp
        commands/ShowCommand.h
        commands/SelectCommand.cpp
        commands/SelectCommand.h
        commands/CommandHandler.cpp
        commands/CommandHandler.h
        "core/Application.cpp"
        "core/Application.h" "cameras/CameraManager.h" "cameras/CameraManager.cpp" "cameras/obsbot/ObsbotCamera.h" "cameras/obsbot/ObsbotCamera.cpp" "cameras/obsbot/ObsbotCameraAdapter.h" "cameras/obsbot/ObsbotCameraAdapter.cpp" "cameras/utils.h" "cameras/CameraState.h" "commands/MoveCommand.h" "commands/MoveCommand.cpp")


# Include directories for headers in obsbot/
target_include_directories(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/obsbot")

# Link against libdev.lib in obsbot/
target_link_libraries(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/obsbot/libdev.lib")

# Copy libdev.dll to the build directory after building
add_custom_command(TARGET ptz_commander POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/obsbot/libdev.dll"
        "$<TARGET_FILE_DIR:ptz_commander>"
    COMMENT "Copying libdev.dll to output directory"
)
