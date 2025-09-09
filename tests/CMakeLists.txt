cmake_minimum_required(VERSION 3.10)
project(dht11_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Coverage option
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Building with code coverage enabled")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

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

if(ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(LCOV_PATH AND GENHTML_PATH)
        message(STATUS "Found lcov and genhtml, coverage targets available")

        # Setup coverage directory
        set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

        # Detect if lcov supports --ignore-errors
        execute_process(
            COMMAND ${LCOV_PATH} --help
            OUTPUT_VARIABLE LCOV_HELP
        )

        set(LCOV_IGNORE "")
        if("${LCOV_HELP}" MATCHES "ignore-errors")
            if("${LCOV_HELP}" MATCHES "mismatch")
                set(LCOV_IGNORE "--ignore-errors mismatch,gcov")
            else()
                set(LCOV_IGNORE "--ignore-errors gcov")
            endif()
        endif()

        add_custom_target(coverage
            # Create coverage directory
            COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
            # Clean previous coverage data
            COMMAND ${LCOV_PATH} --directory . --zerocounters --quiet || true
            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            # Capture coverage data
            COMMAND ${LCOV_PATH} --directory . --capture --output-file ${COVERAGE_DIR}/coverage.info --quiet ${LCOV_IGNORE}
            # Filter out system and test files
            COMMAND ${LCOV_PATH} --remove ${COVERAGE_DIR}/coverage.info '/usr/*' '*/tests/*' '*/gtest/*' '*/gmock/*' --output-file ${COVERAGE_DIR}/coverage.info --quiet --ignore-errors unused
            # Generate HTML report
            COMMAND ${GENHTML_PATH} --demangle-cpp -o ${COVERAGE_DIR}/html ${COVERAGE_DIR}/coverage.info --quiet
            # Show summary
            COMMAND ${LCOV_PATH} --list ${COVERAGE_DIR}/coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS test_dht11
            COMMENT "Generating complete coverage report..."
        )

    else()
        message(WARNING "lcov or genhtml not found, coverage targets not available")
    endif()
endif()
