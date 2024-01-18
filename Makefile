.PHONY: default build rel deb test-rel test-deb clean

MAKE := $(MAKE)
MAKEFLAGS += --no-print-directory
# Adjust parallel build jobs based on your available cores.
JOBS ?= $(shell (command -v nproc > /dev/null 2>&1 && echo "-j$$(nproc)") || echo "")
BUILD_DIR := build/

default: build

rel:
	cmake --preset=rel
	cmake --build $(BUILD_DIR) $(JOBS)

deb:
	cmake --preset=deb
	cmake --build $(BUILD_DIR) $(JOBS)

build:
	cmake --build $(BUILD_DIR) $(JOBS)

format:
	cmake --build $(BUILD_DIR) --target format

tidy:
	cmake --build $(BUILD_DIR) --target tidy $(JOBS)

test-rel: build
	$(BUILD_DIR)rel/tests
	@echo "Ran RELEASE Tests"

test-deb: build
	$(BUILD_DIR)deb/tests
	@echo "Ran DEBUG Tests"

clean:
	rm -rf build/
