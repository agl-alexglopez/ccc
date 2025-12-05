.PHONY: gcc-ccc clang-ccc default build gcc-release gcc-debug clang-release clang-debug sanitize-debug sanitize-release clean tests samples all-gcc-debug all-gcc-release all-sanitize-debug all-sanitize-release all-clang-debug all-clang-release dtest rtest utility tidy format fanalyze

MAKE := $(MAKE)
MAKEFLAGS += --no-print-directory
# Adjust parallel build jobs based on your available cores.
JOBS ?= $(shell (command -v nproc > /dev/null 2>&1 && echo "-j$$(nproc)") || echo "")
BUILD_DIR := build/
PREFIX := install/

ifeq ($(words $(MAKECMDGOALS)),2)
  PREFIX := $(word 2, $(MAKECMDGOALS))
endif

default: build

build:
	cmake --build $(BUILD_DIR) $(JOBS)

gcc-ccc:
	cmake --preset=gcc-release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR) $(JOBS) --target install $(JOBS)

clang-ccc:
	cmake --preset=clang-release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR) $(JOBS) --target install $(JOBS)

install:
	cmake --build $(BUILD_DIR) $(JOBS) --target install $(JOBS)

gcc-release:
	cmake --preset=gcc-release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	$(MAKE) build

gcc-debug:
	cmake --preset=gcc-debug -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	$(MAKE) build

clang-release:
	cmake --preset=clang-release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	$(MAKE) build

clang-debug:
	cmake --preset=clang-debug -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	$(MAKE) build

sanitize-release:
	cmake --preset=gcc-sanitize-release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	$(MAKE) build

sanitize-debug:
	cmake --preset=gcc-sanitize-debug -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	$(MAKE) build

format:
	cmake --build $(BUILD_DIR) $(JOBS) --target format

tidy:
	cmake --build $(BUILD_DIR) $(JOBS) --target tidy $(JOBS)

tests:
	cmake --build $(BUILD_DIR) $(JOBS) --target tests $(JOBS)

samples:
	cmake --build $(BUILD_DIR) $(JOBS) --target samples $(JOBS)

utility:
	cmake --build $(BUILD_DIR) $(JOBS) --target utility $(JOBS)

all-gcc-debug:
	cmake --preset=gcc-debug -DCMAKE_INSTALL_PREFIX=$(PREFIX) && cmake --build build $(JOBS) --target ccc tests samples

all-gcc-release:
	cmake --preset=gcc-release -DCMAKE_INSTALL_PREFIX=$(PREFIX) && cmake --build build $(JOBS) --target ccc tests samples

all-sanitize-debug:
	cmake --preset=gcc-sanitize-debug -DCMAKE_INSTALL_PREFIX=$(PREFIX) && cmake --build build $(JOBS) --target ccc tests samples

all-sanitize-release:
	cmake --preset=gcc-sanitize-release -DCMAKE_INSTALL_PREFIX=$(PREFIX) && cmake --build build $(JOBS) --target ccc tests samples

all-clang-debug:
	cmake --preset=clang-debug -DCMAKE_INSTALL_PREFIX=$(PREFIX) && cmake --build build $(JOBS) --target ccc tests samples

all-clang-release:
	cmake --preset=clang-release -DCMAKE_INSTALL_PREFIX=$(PREFIX) && cmake --build build $(JOBS) --target ccc tests samples

dtest: tests
	cmake --build build $(JOBS) --target tests
	$(BUILD_DIR)debug/bin/run_tests $(BUILD_DIR)debug/bin/tests/
	@echo "RAN TESTS"

rtest: tests
	cmake --build build $(JOBS) --target tests
	$(BUILD_DIR)bin/run_tests $(BUILD_DIR)bin/tests/
	@echo "RAN TESTS"

clean:
	rm -rf build/ install/
