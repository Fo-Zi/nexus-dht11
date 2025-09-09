cmake_minimum_required(VERSION 3.10)
project(dht11_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# HAL interface path - easily configurable
set(HAL_INTERFACE_PATH "../../hal-interface" CACHE STRING "Path to hal-interface directory")

# Find required packages
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Add the main DHT11 driver source
add_library(dht11_lib
    ../src/dht11.c
)

target_include_directories(dht11_lib
    PUBLIC
        ../include
        ${HAL_INTERFACE_PATH}/include
)

# Add Pin mock and common timing mocks for DHT11 driver
add_library(nhal_dht11_mocks
    ${HAL_INTERFACE_PATH}/testing/gtest_mocks/src/nhal_pin_mock.cpp
    ${HAL_INTERFACE_PATH}/testing/gtest_mocks/src/nhal_common_mock.cpp
)

target_include_directories(nhal_dht11_mocks
    PUBLIC
        ${HAL_INTERFACE_PATH}/testing/gtest_mocks/include
        ${HAL_INTERFACE_PATH}/include
)

target_link_libraries(nhal_dht11_mocks
    PUBLIC
        GTest::gmock
        GTest::gtest
)

# Create test executable
add_executable(test_dht11
    test_dht11_init.cpp
    test_dht11_read.cpp
    test_dht11_utils.cpp
)

target_link_libraries(test_dht11
    PRIVATE
        dht11_lib
        nhal_dht11_mocks
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

target_include_directories(test_dht11
    PRIVATE
        ../include
        ${HAL_INTERFACE_PATH}/include
        ${HAL_INTERFACE_PATH}/testing/gtest_mocks/include
)

# Enable testing
enable_testing()
add_test(NAME DHT11Tests COMMAND test_dht11)
