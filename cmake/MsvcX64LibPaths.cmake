# Library names on the link line have no directory. Ninja then uses LIB, and a
# 32-bit ambient LIB supplies x86 import libraries to an x64 link. Pin the x64
# libs that belong to the compiler CMake already selected. The Visual Studio
# generator already injects those paths; CMAKE_RC_COMPILER is often just rc.exe
# there, so walking it produces a bogus /lib//um/x64.
function(ptz_pin_msvc_x64_libs target_name)
    if(NOT WIN32 OR NOT MSVC)
        return()
    endif()

    get_filename_component(_ptz_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_ptz_arch "${_ptz_cl_dir}" NAME)
    if(NOT _ptz_arch STREQUAL "x64")
        message(FATAL_ERROR
            "${target_name} is built for x64. CMAKE_CXX_COMPILER is ${CMAKE_CXX_COMPILER}")
    endif()
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        return()
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
    target_link_directories(${target_name} BEFORE PRIVATE
        "${_ptz_msvc_lib}"
        "${_ptz_sdk_um}"
        "${_ptz_sdk_ucrt}"
    )
endfunction()
