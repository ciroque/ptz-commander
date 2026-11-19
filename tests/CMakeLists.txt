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
FetchContent_MakeAvailable(googletest)

add_executable(ptz_tests
    SplitArgsTest.cpp
    ViscaRangeTest.cpp
    ViscaIpFramingTest.cpp
    "${CMAKE_SOURCE_DIR}/cameras/visca/ViscaCommands.cpp"
)

target_include_directories(ptz_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/commands"
    "${CMAKE_SOURCE_DIR}/cameras/visca"
)

target_link_libraries(ptz_tests PRIVATE GTest::gtest_main)
target_compile_features(ptz_tests PRIVATE cxx_std_20)

include(GoogleTest)
# PRE_TEST discovers cases when ctest runs, which works for the Visual Studio
# multi-config generator. Configure time would need the exe already built.
gtest_discover_tests(ptz_tests DISCOVERY_MODE PRE_TEST)
