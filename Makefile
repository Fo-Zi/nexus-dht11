# DHT11 Driver Makefile
# Provides shortcuts for common development tasks

.PHONY: help config_tests run_unit_tests clean_unit_tests ci_local update_deps

help:
	@echo "Available targets:"
	@echo "  config_tests    - Configure CMake build for tests"
	@echo "  run_unit_tests  - Build and run unit tests"
	@echo "  clean_unit_tests - Clean test build directory"
	@echo "  ci_local        - Run full CI pipeline locally"
	@echo "  update_deps     - Update West dependencies"
	@echo "  help            - Show this help message"

update_deps:
	west update

config_tests:
	cd tests && cmake -B build

run_unit_tests: config_tests
	cd tests && cmake --build build && ctest --test-dir build --output-on-failure --verbose

clean_unit_tests:
	cd tests && rm -rf build

ci_local: clean_unit_tests update_deps run_unit_tests
	@echo "✅ CI pipeline completed successfully!"