cmake_minimum_required(VERSION 3.24)
project(SplitFlapUnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SPLITFLAP_SANITIZERS "Run host tests with address and undefined-behavior checks" OFF)
if(SPLITFLAP_SANITIZERS)
    if(WIN32 OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer tests require GCC or Clang on Linux/macOS")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

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

# Exercise the actual MQTT implementation and protocol library. Only the
# transport and unrelated application/Arduino services are substituted.
FetchContent_Declare(
    pubsubclient3
    URL https://github.com/hmueller01/pubsubclient3/archive/a69cc5c44fffda7e07963863dcffdeb4384f0579.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(pubsubclient3)
configure_file(../src/SplitFlapMqtt.cpp mqtt/SplitFlapMqtt.cpp COPYONLY)
configure_file(../src/SplitFlapMqtt.h mqtt/SplitFlapMqtt.h COPYONLY)
add_library(firmware_mqtt STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/mqtt/SplitFlapMqtt.cpp"
    "${pubsubclient3_SOURCE_DIR}/src/PubSubClient.cpp"
)
target_include_directories(firmware_mqtt PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/mqtt"
    support/mqtt
    "${pubsubclient3_SOURCE_DIR}/src"
    ../src
)
add_executable(mqtt_recovery_test unit/MqttRecoveryTest.cpp)
target_link_libraries(mqtt_recovery_test PRIVATE firmware_mqtt GTest::gtest_main)
gtest_discover_tests(mqtt_recovery_test)

# ArduinoJson v7.4.3, pinned to the commit targeted by its annotated release tag.
FetchContent_Declare(arduinojson
    URL https://github.com/bblanchon/ArduinoJson/archive/77771d3c07668e01d8f52acb03910c1110bb373f.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(arduinojson)

# Each component target gets its real headers; unrelated boundary stand-ins are
# kept in a separate include directory from the sketch and MQTT doubles.
foreach(variant IN ITEMS single dual)
    set(component_dir "${CMAKE_CURRENT_BINARY_DIR}/components-${variant}")
    foreach(file IN ITEMS JsonSetting.cpp JsonSetting.h JsonSettings.cpp JsonSettings.h
            SplitFlapModule.cpp SplitFlapModule.h SplitFlapDisplay.cpp SplitFlapDisplay.h
            SplitFlapWebServer.cpp SplitFlapWebServer.h)
        configure_file("../src/${file}" "${component_dir}/${file}" COPYONLY)
    endforeach()
    add_library(components_${variant} STATIC
        "${component_dir}/JsonSetting.cpp" "${component_dir}/JsonSettings.cpp"
        "${component_dir}/SplitFlapModule.cpp" "${component_dir}/SplitFlapDisplay.cpp"
        "${component_dir}/SplitFlapWebServer.cpp"
        support/firmware/Background.cpp)
    target_include_directories(components_${variant} PUBLIC "${component_dir}"
        support/firmware "${arduinojson_SOURCE_DIR}/src" ../src)
    target_compile_definitions(components_${variant} PUBLIC
        ARDUINOJSON_ENABLE_ARDUINO_STRING=1 ARDUINOJSON_ENABLE_ARDUINO_STREAM=0
        ARDUINOJSON_ENABLE_ARDUINO_PRINT=0 ARDUINOJSON_ENABLE_PROGMEM=0)
    if(variant STREQUAL "dual")
        target_compile_definitions(components_${variant} PUBLIC ENABLE_DUAL_I2C=1)
    endif()
    add_executable(firmware_behavior_${variant} unit/SettingsTest.cpp unit/ModuleTest.cpp unit/DisplayTest.cpp unit/WebCommandTest.cpp)
    target_link_libraries(firmware_behavior_${variant} PRIVATE components_${variant} GTest::gtest_main)
    gtest_discover_tests(firmware_behavior_${variant} TEST_PREFIX "${variant}.")
    add_executable(sketch_behavior_${variant} unit/SketchBehaviorTest.cpp)
    target_include_directories(sketch_behavior_${variant} PRIVATE support
        "${CMAKE_CURRENT_BINARY_DIR}/sketch" ../src)
    target_compile_definitions(sketch_behavior_${variant} PRIVATE SERIAL_SPEED=115200 SDA_PIN=8 SCL_PIN=9)
    if(variant STREQUAL "dual")
        target_compile_definitions(sketch_behavior_${variant} PRIVATE ENABLE_DUAL_I2C=1 SDA2_PIN=10 SCL2_PIN=11)
    endif()
    target_link_libraries(sketch_behavior_${variant} PRIVATE GTest::gtest_main)
    gtest_discover_tests(sketch_behavior_${variant} TEST_PREFIX "${variant}.")
endforeach()
