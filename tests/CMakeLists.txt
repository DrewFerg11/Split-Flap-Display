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

add_executable(app_entry_point_test
    unit/AppEntryPointTest.cpp
    ../src/AppLoop.cpp
)
# Substitutes are private to this target, ahead of the production headers.
target_include_directories(app_entry_point_test PRIVATE support ../src)
target_link_libraries(app_entry_point_test PRIVATE GTest::gtest_main)
if(MSVC)
    target_compile_options(app_entry_point_test PRIVATE /W4 /WX)
else()
    target_compile_options(app_entry_point_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
gtest_discover_tests(app_entry_point_test)
