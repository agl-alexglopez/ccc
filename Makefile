.PHONY: ccc ccc-deb default build gcc-rel gcc-deb clang-rel clang-deb dsan rsan clean tests samples all-gcc-deb all-gcc-rel all-dsan all-rsan all-clang-deb all-clang-rel dtest rtest util tidy format fanalyze

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

ccc:
	cmake --preset=gcc-rel -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR) --target install $(JOBS)

ccc-deb:
	cmake --preset=gcc-deb -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR) --target install $(JOBS)

install:
	cmake --build $(BUILD_DIR) --target install $(JOBS)

gcc-rel:
	cmake --preset=gcc-rel
	$(MAKE) build

gcc-deb:
	cmake --preset=gcc-deb
	$(MAKE) build

clang-rel:
	cmake --preset=clang-rel
	$(MAKE) build

clang-deb:
	cmake --preset=clang-deb
	$(MAKE) build

rsan:
	cmake --preset=rsan
	$(MAKE) build

dsan:
	cmake --preset=dsan
	$(MAKE) build

format:
	cmake --build $(BUILD_DIR) --target format

tidy:
	cmake --build $(BUILD_DIR) --target tidy $(JOBS)

tests:
	cmake --build $(BUILD_DIR) --target tests $(JOBS)

samples:
	cmake --build $(BUILD_DIR) --target samples $(JOBS)

util:
	cmake --build $(BUILD_DIR) --target util $(JOBS)

all-gcc-deb:
	$(MAKE) gcc-deb
	$(MAKE) tests
	$(MAKE) samples

all-gcc-rel:
	$(MAKE) gcc-rel
	$(MAKE) tests
	$(MAKE) samples

all-dsan:
	$(MAKE) dsan
	$(MAKE) tests
	$(MAKE) samples

all-rsan:
	$(MAKE) rsan
	$(MAKE) tests
	$(MAKE) samples

all-clang-deb:
	$(MAKE) clang-deb
	$(MAKE) tests
	$(MAKE) samples

all-clang-rel:
	$(MAKE) clang-rel
	$(MAKE) tests
	$(MAKE) samples

dtest: tests
	$(BUILD_DIR)debug/bin/run_tests $(BUILD_DIR)debug/bin/tests/
	@echo "RAN TESTS"

rtest: tests
	$(BUILD_DIR)bin/run_tests $(BUILD_DIR)bin/tests/
	@echo "RAN TESTS"

clean:
	rm -rf build/ install/
