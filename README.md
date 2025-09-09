![GitHub License](https://img.shields.io/github/license/Fo-Zi/nexus-dht11?color=lightgrey)
![GitHub Release](https://img.shields.io/github/v/release/Fo-Zi/nexus-dht11?color=brightgreen)
[![CI](https://github.com/fo-zi/nexus-dht11/workflows/DHT11%20Driver%20CI/badge.svg)](https://github.com/fo-zi/nexus-dht11/actions)
[![codecov](https://codecov.io/gh/fo-zi/nexus-dht11/branch/main/graph/badge.svg)](https://codecov.io/gh/fo-zi/nexus-dht11)
[![Dependency](https://img.shields.io/badge/depends%20on-nexus--hal--interface%20v0.6.0-orange)](https://github.com/Fo-Zi/nexus-hal-interface/tree/v0.6.0)

# DHT11 Driver

A C driver for the DHT11 temperature and humidity sensor, part of the [Nexus Ecosystem](https://github.com/Fo-Zi/nexus-embedded-ecosystem)

## Overview

This driver provides temperature and humidity readings from the DHT11 sensor using a single-wire communication protocol. It handles the precise timing requirements and protocol implementation automatically.

## Hardware

- **Device**: DHT11 Temperature & Humidity Sensor
- **Interface**: Single-wire digital protocol
- **Measurement Range**:
  - Humidity: 20-90% RH
  - Temperature: 0-50°C
- **Accuracy**: ±5% RH, ±2°C
- **Power**: 3.0-5.5V

## Features

- Temperature and humidity readings
- Automatic timing and protocol handling
- Built-in data validation with checksum verification
- Rate limiting (minimum 2 seconds between readings)
- Error reporting and validation

## Building

```bash
# Build as part of West workspace
west build

# Or include in CMake project
add_subdirectory(dht11)
```

## Usage

1. Initialize an NHAL pin context for the data pin
2. Initialize the DHT11 handle with `dht11_init()`
3. Use `dht11_read()` for processed readings or `dht11_read_raw()` for raw data
4. Wait at least 2 seconds between readings

See the header file for detailed function documentation.

## Dependencies

- NHAL pin interface
- Standard C library

## Development

### Testing

Run the unit tests:

```bash
make run_unit_tests
```

### Code Coverage

Generate local coverage report:

```bash
make run_coverage
```

This will generate an HTML coverage report at `tests/build-coverage/coverage/html/index.html`.

#### Requirements for Coverage
- `lcov` and `genhtml` tools
- GCC compiler with coverage support

Install on Ubuntu/Debian:
```bash
sudo apt install lcov
```

### Available Makefile Targets

- `make config_tests` - Configure CMake build for tests
- `make run_unit_tests` - Build and run unit tests
- `make clean_unit_tests` - Clean test build directory
- `make config_coverage` - Configure CMake build with coverage enabled
- `make run_coverage` - Build, run tests, and generate coverage report
- `make clean_coverage` - Clean coverage build directory
- `make ci_local` - Run full CI pipeline locally
- `make update_deps` - Update West dependencies

### Coverage Reports

Coverage reports are automatically generated in CI and uploaded to [Codecov](https://codecov.io/gh/fo-zi/nexus-dht11). The current coverage target is 80%.
