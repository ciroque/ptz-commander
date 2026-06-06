cmake_minimum_required(VERSION 3.28)
project(ptz_commander VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

add_executable(ptz_commander main.cpp)

# Modular source organization via subdirectories + target_sources
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(cameras)
add_subdirectory(commands)

# Include directories for third-party headers (nlohmann/json, obsbot SDK, spdlog)
target_include_directories(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/include/" "${CMAKE_SOURCE_DIR}")

# Link against obsbot SDK
target_link_libraries(ptz_commander PRIVATE "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.lib")

# VISCA serial discovery uses SetupAPI on Windows (VID/PID enumeration for Keyspan etc.)
if(WIN32)
    target_link_libraries(ptz_commander PRIVATE setupapi)

    # Embed the application icon into the executable (ptz-commander.rc references ptz-commander.ico).
    # We copy the .ico into the build dir because the resource compiler runs out-of-source
    # and the .rc uses a relative path.
    #
    # For sharp "Extra large icons" (256x256) in Explorer, the ptz-commander.ico file
    # **must** contain a 256x256 image stored as PNG (not raw BMP). This is the modern
    # way to get high-quality large icons from a .ico resource.
    #
    # Recommended way to (re)generate a good .ico from your source PNG (requires ImageMagick):
    #   magick ptz-commander.png -define icon:auto-resize=256,128,64,48,32,16 ptz-commander.ico
    #
    # The 256x256 layer will be stored as PNG inside the .ico, giving you the same
    # quality you saw when directly referencing the PNG.
    target_sources(ptz_commander PRIVATE ptz-commander.rc)
    configure_file(ptz-commander.ico ptz-commander.ico COPYONLY)
    set_source_files_properties(ptz-commander.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/ptz-commander.ico"
    )
endif()

# Copy libdev.dll to the build directory after building (so the exe can find it at runtime)
add_custom_command(TARGET ptz_commander POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.dll"
        "$<TARGET_FILE_DIR:ptz_commander>"
    COMMENT "Copying libdev.dll to output directory"
)

# Installation rules (used by `cmake --install` and by the WiX installer build)
install(TARGETS ptz_commander RUNTIME DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.dll" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/ptz-commander.ico" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/installer/License.rtf" DESTINATION .)

# CPack configuration for Windows (WiX MSI generator).
# This block is guarded so it has no effect on non-Windows platforms.
if(WIN32)
    # Package metadata (used by any CPack generator)
    set(CPACK_PACKAGE_NAME "PTZ Commander")
    set(CPACK_PACKAGE_VENDOR "PTZ Commander Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Command-line tool for controlling PTZ cameras via VISCA and OBSBOT SDK")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "PTZCommander")

    # WiX-specific options. These only matter when you actually generate an MSI.
    set(CPACK_WIX_UPGRADE_GUID "6B565268-915A-461E-8EBF-F154B835455D")
    set(CPACK_WIX_PRODUCT_GUID "*")

    # Do NOT force CPACK_GENERATOR here. Users / CI control the generator:
    #   cmake -B build -DCPACK_GENERATOR=WIX ...
    #   cpack
    # or simply: cpack -G WIX
    include(CPack)
endif()
