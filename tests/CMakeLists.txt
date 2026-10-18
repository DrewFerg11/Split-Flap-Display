cmake_minimum_required(VERSION 3.24)
project(SplitFlapUnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
# GoogleTest v1.17.0; pin the commit for reproducible test builds.
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# Mirror the sketch byte-for-byte so its quoted includes can resolve to test
# substitutes rather than adjacent production headers. configure_file tracks
# source changes and refreshes this copy automatically; no code is rewritten.
configure_file(../src/SplitFlapDisplay.ino sketch/SplitFlapDisplay.ino COPYONLY)
set(sketch_source "${CMAKE_CURRENT_BINARY_DIR}/sketch/SplitFlapDisplay.ino")
set_source_files_properties("${sketch_source}" PROPERTIES LANGUAGE CXX)
add_library(firmware_sketch STATIC "${sketch_source}")
target_include_directories(firmware_sketch PRIVATE support ../src)
target_compile_definitions(firmware_sketch PRIVATE SERIAL_SPEED=115200 SDA_PIN=8 SCL_PIN=9)
# Arduino normally generates these declarations before compiling the sketch.
if(MSVC)
    target_compile_options(firmware_sketch PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/support/SketchDeclarations.h")
else()
    target_compile_options(firmware_sketch PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/support/SketchDeclarations.h")
endif()

add_executable(app_entry_point_test unit/AppEntryPointTest.cpp)
# Substitutes are private to these targets; PlatformIO never sees them.
target_include_directories(app_entry_point_test PRIVATE support ../src)
target_link_libraries(app_entry_point_test PRIVATE firmware_sketch GTest::gtest_main)
if(MSVC)
    target_compile_options(app_entry_point_test PRIVATE /W4 /WX)
else()
    target_compile_options(app_entry_point_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
gtest_discover_tests(app_entry_point_test)
