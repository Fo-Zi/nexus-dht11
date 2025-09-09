# DHT11 Driver Makefile
# Provides shortcuts for common development tasks

.PHONY: help config_tests run_unit_tests clean_unit_tests ci_local update_deps config_coverage run_coverage clean_coverage

help:
	@echo "Available targets:"
	@echo "  config_tests     - Configure CMake build for tests"
	@echo "  run_unit_tests   - Build and run unit tests"
	@echo "  clean_unit_tests - Clean test build directory"
	@echo "  config_coverage  - Configure CMake build with coverage enabled"
	@echo "  run_coverage     - Build, run tests, and generate coverage report"
	@echo "  clean_coverage   - Clean coverage build directory"
	@echo "  ci_local         - Run full CI pipeline locally"
	@echo "  update_deps      - Update West dependencies"
	@echo "  help             - Show this help message"

update_deps:
	west update

config_tests:
	cd tests && cmake -B build

run_unit_tests: config_tests
	cd tests && cmake --build build && ctest --test-dir build --output-on-failure --verbose

clean_unit_tests:
	cd tests && rm -rf build

config_coverage:
	cd tests && cmake -B build-coverage -DENABLE_COVERAGE=ON

run_coverage: config_coverage
	cd tests && cmake --build build-coverage && cd build-coverage && make coverage
	@echo "✅ Coverage report generated at tests/build-coverage/coverage/html/index.html"

clean_coverage:
	cd tests && rm -rf build-coverage

ci_local: clean_unit_tests update_deps run_unit_tests
	@echo "✅ CI pipeline completed successfully!"
