.PHONY: ccc default build rel deb crel cdeb clean tests samples all-deb all-rel call-deb call-rel dtest rtest util tidy format fanalyze

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
	cmake --preset=rel -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR) --target install $(JOBS)

install:
	cmake --build $(BUILD_DIR) --target install $(JOBS)

rel:
	cmake --preset=rel
	$(MAKE) build

deb:
	cmake --preset=deb
	$(MAKE) build

crel:
	cmake --preset=crel
	$(MAKE) build

cdeb:
	cmake --preset=cdeb
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

all-deb:
	$(MAKE) deb
	$(MAKE) tests
	$(MAKE) samples

all-rel:
	$(MAKE) rel
	$(MAKE) tests
	$(MAKE) samples

fanalyze:
	cmake --preset=fanalyze
	$(MAKE) build
	$(MAKE) tests
	$(MAKE) samples

call-deb:
	$(MAKE) cdeb
	$(MAKE) tests
	$(MAKE) samples

call-rel:
	$(MAKE) crel
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
