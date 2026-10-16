cmake_minimum_required(VERSION 3.28)

# Stamp before project() so PROJECT_VERSION matches the git tag, not a hardcoded 1.0.0.
set(PTZ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PTZ_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/StampVersion.cmake")
ptz_stamp_version()

project(ptz_commander VERSION ${PTZ_VERSION_MAJOR}.${PTZ_VERSION_MINOR}.${PTZ_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 20)

add_executable(ptz_commander main.cpp)

# Modular source organization via subdirectories + target_sources
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(cameras)
add_subdirectory(commands)

# IXWebSocket (localhost scene list/apply). TLS/zlib off: we only need an unencrypted
# localhost socket, and Windows agents do not ship zlib/OpenSSL by default.
include(FetchContent)
set(USE_TLS OFF CACHE BOOL "IXWebSocket TLS" FORCE)
set(USE_ZLIB OFF CACHE BOOL "IXWebSocket zlib" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "Install IXWebSocket" FORCE)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v12.0.1
    GIT_SHALLOW TRUE
)
# IXWebSocket's CMakeLists sets CMAKE_CXX_STANDARD 11; keep our C++20.
set(_ptz_cxx_standard ${CMAKE_CXX_STANDARD})
FetchContent_MakeAvailable(ixwebsocket)
set(CMAKE_CXX_STANDARD ${_ptz_cxx_standard})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static ixwebsocket keeps Winsock as PRIVATE; pull them through to the exe.
if(WIN32)
    target_link_libraries(ixwebsocket PUBLIC wsock32 ws2_32 shlwapi)
endif()

# Include directories for third-party headers (nlohmann/json, obsbot SDK, spdlog)
# and the generated version stamp (ptz_version.h).
target_include_directories(ptz_commander PRIVATE
    "${CMAKE_SOURCE_DIR}/include/"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/generated"
)

# Refresh the stamp every build so the binary tracks HEAD without a reconfigure.
# copy_if_different keeps the header timestamp stable when the version did not change.
add_custom_target(ptz_version_stamp
    COMMAND ${CMAKE_COMMAND}
        -DPTZ_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DPTZ_BINARY_DIR=${CMAKE_BINARY_DIR}
        -DPTZ_VERSION_OVERRIDE=${PTZ_VERSION}
        -P ${CMAKE_SOURCE_DIR}/cmake/StampVersion.cmake
    BYPRODUCTS
        ${CMAKE_BINARY_DIR}/generated/ptz_version.h
        ${CMAKE_BINARY_DIR}/ptz_product_version.txt
    COMMENT "Stamping PTZ Commander version"
    VERBATIM
)
add_dependencies(ptz_commander ptz_version_stamp)

# Link against obsbot SDK and IXWebSocket
target_link_libraries(ptz_commander PRIVATE
    "${CMAKE_SOURCE_DIR}/lib/obsbot/libdev.lib"
    ixwebsocket
)

# VISCA serial discovery uses SetupAPI on Windows (VID/PID enumeration for Keyspan etc.)
if(WIN32)
    target_link_libraries(ptz_commander PRIVATE setupapi winhttp)

    # Library names on the link line have no directory. The linker then uses LIB,
    # and a 32-bit LIB (this machine's ambient environment) supplies x86 import
    # libraries to an x64 link. /LIBPATH is searched first, so pin the x64 libs
    # that belong to the compiler CMake already selected.
    if(MSVC)
        get_filename_component(_ptz_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(_ptz_arch "${_ptz_cl_dir}" NAME)
        if(NOT _ptz_arch STREQUAL "x64")
            message(FATAL_ERROR
                "ptz-commander links the x64 OBSBOT library. CMAKE_CXX_COMPILER is ${CMAKE_CXX_COMPILER}")
        endif()
        get_filename_component(_ptz_msvc_host "${_ptz_cl_dir}" DIRECTORY)
        get_filename_component(_ptz_msvc_bin "${_ptz_msvc_host}" DIRECTORY)
        get_filename_component(_ptz_msvc_root "${_ptz_msvc_bin}" DIRECTORY)
        set(_ptz_msvc_lib "${_ptz_msvc_root}/lib/x64")

        get_filename_component(_ptz_rc_dir "${CMAKE_RC_COMPILER}" DIRECTORY)
        get_filename_component(_ptz_sdk_ver_dir "${_ptz_rc_dir}" DIRECTORY)
        get_filename_component(_ptz_sdk_ver "${_ptz_sdk_ver_dir}" NAME)
        get_filename_component(_ptz_sdk_bin "${_ptz_sdk_ver_dir}" DIRECTORY)
        get_filename_component(_ptz_sdk_root "${_ptz_sdk_bin}" DIRECTORY)
        set(_ptz_sdk_um "${_ptz_sdk_root}/lib/${_ptz_sdk_ver}/um/x64")
        set(_ptz_sdk_ucrt "${_ptz_sdk_root}/lib/${_ptz_sdk_ver}/ucrt/x64")

        foreach(_ptz_libdir IN ITEMS "${_ptz_msvc_lib}" "${_ptz_sdk_um}" "${_ptz_sdk_ucrt}")
            if(NOT EXISTS "${_ptz_libdir}")
                message(FATAL_ERROR "x64 library directory not found: ${_ptz_libdir}")
            endif()
        endforeach()
        target_link_directories(ptz_commander BEFORE PRIVATE
            "${_ptz_msvc_lib}"
            "${_ptz_sdk_um}"
            "${_ptz_sdk_ucrt}"
        )
    endif()

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
    # rc.exe looks up the icon relative to the build dir, and does not reliably
    # inherit the target include path for the generated version header.
    set_source_files_properties(ptz-commander.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/ptz-commander.ico;${CMAKE_BINARY_DIR}/generated/ptz_version.h"
        COMPILE_FLAGS "/I\"${CMAKE_BINARY_DIR}/generated\""
    )
    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/commands/console/VersionCommand.cpp"
        "${CMAKE_SOURCE_DIR}/commands/console/ClearCommand.cpp"
        "${CMAKE_SOURCE_DIR}/core/Application.cpp"
        PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/generated/ptz_version.h"
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
install(FILES
    "${CMAKE_SOURCE_DIR}/obs-panel/index.html"
    "${CMAKE_SOURCE_DIR}/obs-panel/GETTING_STARTED.html"
    DESTINATION obs-panel
)

# CPack configuration for Windows (WiX MSI generator).
# This block is guarded so it has no effect on non-Windows platforms.
if(WIN32)
    # Package metadata (used by any CPack generator)
    set(CPACK_PACKAGE_NAME "PTZ Commander")
    set(CPACK_PACKAGE_VENDOR "PTZ Commander Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Command-line tool for controlling PTZ cameras via VISCA and OBSBOT SDK")
    set(CPACK_PACKAGE_VERSION "${PTZ_VERSION_MSI}")
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
