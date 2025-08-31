cmake_minimum_required(VERSION 3.10)
project(dht11_driver VERSION 1.0.0)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# HAL interface path - configurable
set(HAL_INTERFACE_PATH "../hal-interface" CACHE STRING "Path to hal-interface directory")

# Create DHT11 driver library
add_library(dht11
    src/dht11.c
)

target_include_directories(dht11
    PUBLIC
        include
        ${HAL_INTERFACE_PATH}/include
)

# Set library properties
set_target_properties(dht11 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Install rules (optional)
install(TARGETS dht11
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Testing (optional - enable if building tests)
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()