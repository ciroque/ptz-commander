cmake_minimum_required(VERSION 3.28)
project(ptz_commander)

set(CMAKE_CXX_STANDARD 20)

add_executable(ptz_commander main.cpp
        commands/Command.cpp
        commands/Command.h
        "commands/camera/ListCommand.cpp"
        "commands/camera/ListCommand.h"
        
        data/Context.h
        "commands/camera/ShowCommand.cpp"
        "commands/camera/ShowCommand.h"
        
        
        commands/CommandHandler.cpp
        commands/CommandHandler.h
        "core/Application.cpp"
        "core/Application.h" 
        "cameras/CameraManager.h" 
        "cameras/CameraManager.cpp" 
        "cameras/obsbot/ObsbotCamera.h" 
        "cameras/obsbot/ObsbotCamera.cpp" 
        "cameras/obsbot/ObsbotCameraAdapter.h" 
        "cameras/obsbot/ObsbotCameraAdapter.cpp" 
        "cameras/utils.h" "cameras/Preset.h" 
        "commands/camera/MoveCommand.h" 
        "commands/camera/MoveCommand.cpp" 
        "commands/preset/StoreCommand.h" 
        "commands/preset/StoreCommand.cpp" 
        "commands/preset/DiscardCommand.h" 
        "commands/preset/DiscardCommand.cpp" 
        "commands/preset/ListCommand.h" 
        "commands/preset/ListCommand.cpp" 
        "commands/preset/ApplyCommand.h" 
        "commands/preset/ApplyCommand.cpp" 
        "commands/preset/SaveCommand.h" 
        "commands/preset/SaveCommand.cpp" 
        "commands/preset/LoadCommand.h" 
        "commands/preset/LoadCommand.cpp" 
        "commands/camera/AliasCommand.h" 
        "commands/camera/AliasCommand.cpp"
        "commands/console/ClearCommand.h"
        "commands/console/ClearCommand.cpp"
)


# Include directories for headers in obsbot/
target_include_directories(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/include/")

# Link against libdev.lib in obsbot/
target_link_libraries(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.lib")

# Copy libdev.dll to the build directory after building
add_custom_command(TARGET ptz_commander POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.dll"
        "$<TARGET_FILE_DIR:ptz_commander>"
    COMMENT "Copying libdev.dll to output directory"
)
