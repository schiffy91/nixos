# Use the compiler pinned in flake.lock by default. Override BTRC=... when
# intentionally testing a local btrcpy checkout.
BTRC ?= nix run --no-warn-dirty --inputs-from . btrc\#btrcpy --
PYTHON ?= python3
CC ?= cc
CFLAGS ?= -std=c11 -pedantic
# Programs are transpiled program-only with strict imports and reference the
# prebuilt stdlib archive (built by `--build-stdlib`); they include
# btrc_stdlib.h and link libbtrc.a.
TRANSPILER_FLAGS ?= --no-stdlib --strict-imports --stdlib "$(CURDIR)/$(STDLIB_DIR)"
NIXOSCTL_LIBS ?= -lm -lpthread -lutil
IMMUTABILITY_LIBS ?= -lm -lpthread

BUILD_DIR := build
STDLIB_DIR := $(BUILD_DIR)/stdlib
STDLIB_HEADER := $(STDLIB_DIR)/btrc_stdlib.h
STDLIB_IMPL := $(STDLIB_DIR)/btrc_stdlib.c
STDLIB_MANIFEST := $(STDLIB_DIR)/btrc_stdlib.manifest
STDLIB_LIB := $(BUILD_DIR)/libbtrc.a
BTRC_STAMP := $(BUILD_DIR)/.btrc-tool
NIXOSCTL_ENTRY := btrc/nixosctl/nixosctl.btrc
IMMUTABILITY_ENTRY := btrc/immutability/immutability.btrc
NIXOSCTL_C_OUT := $(BUILD_DIR)/nixosctl.c
IMMUTABILITY_C_OUT := $(BUILD_DIR)/immutability.c
NIXOSCTL_BIN := $(BUILD_DIR)/nixosctl
IMMUTABILITY_BIN := $(BUILD_DIR)/immutability
IMMUTABILITY_PATHS_TEST_ENTRY := tests/unit/immutability_paths.btrc
IMMUTABILITY_PATHS_TEST_C_OUT := $(BUILD_DIR)/immutability_paths_test.c
IMMUTABILITY_PATHS_TEST_BIN := $(BUILD_DIR)/immutability_paths_test
BIN := $(NIXOSCTL_BIN)
BINS := $(NIXOSCTL_BIN) $(IMMUTABILITY_BIN)
SOURCES := $(shell find btrc tests/e2e tests/unit -name '*.btrc' | sort)

.PHONY: all transpile build-stdlib build unit check quick smoke test host-smoke app-settings stateful-host x86_64-qemu-host aarch64-qemu-host installer-download install-system immutability-reset immutability-key-encoding installer-ssh installer-ssh-smoke tpm2-probe secure-boot-capabilities secure-boot-install secure-boot-lanzaboote graph-list graph-status graph-coverage graph-early graph-installer-ssh graph-full chain clean dirs

all: build

dirs:
	mkdir -p $(BUILD_DIR)

$(BTRC_STAMP): | dirs
	@tmp="$@.tmp"; \
	printf '%s\n' "$(BTRC)" > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

# Compile the stdlib once into a reusable archive (header + impl + manifest),
# then a static library for local linking.
build-stdlib: $(STDLIB_LIB)

$(STDLIB_MANIFEST): $(SOURCES) flake.lock $(BTRC_STAMP) | dirs
	$(BTRC) --build-stdlib "$(CURDIR)/$(STDLIB_DIR)"

$(STDLIB_LIB): $(STDLIB_MANIFEST)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -c "$(CURDIR)/$(STDLIB_IMPL)" -o "$(CURDIR)/$(BUILD_DIR)/btrc_stdlib.o"
	ar rcs "$(CURDIR)/$(STDLIB_LIB)" "$(CURDIR)/$(BUILD_DIR)/btrc_stdlib.o"

transpile: $(NIXOSCTL_C_OUT) $(IMMUTABILITY_C_OUT)

$(NIXOSCTL_C_OUT): $(SOURCES) $(STDLIB_MANIFEST) $(BTRC_STAMP) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(NIXOSCTL_ENTRY)" -o "$(CURDIR)/$(NIXOSCTL_C_OUT)"

$(IMMUTABILITY_C_OUT): $(SOURCES) $(STDLIB_MANIFEST) $(BTRC_STAMP) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(IMMUTABILITY_ENTRY)" -o "$(CURDIR)/$(IMMUTABILITY_C_OUT)"

$(IMMUTABILITY_PATHS_TEST_C_OUT): $(SOURCES) $(STDLIB_MANIFEST) $(BTRC_STAMP) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(IMMUTABILITY_PATHS_TEST_ENTRY)" -o "$(CURDIR)/$(IMMUTABILITY_PATHS_TEST_C_OUT)"

build: $(BINS)

$(NIXOSCTL_BIN): $(NIXOSCTL_C_OUT) $(STDLIB_LIB)
	$(CC) $(CFLAGS) -I"$(CURDIR)/$(STDLIB_DIR)" "$(CURDIR)/$(NIXOSCTL_C_OUT)" "$(CURDIR)/$(STDLIB_LIB)" -o "$(CURDIR)/$(NIXOSCTL_BIN)" $(NIXOSCTL_LIBS)

$(IMMUTABILITY_BIN): $(IMMUTABILITY_C_OUT) $(STDLIB_LIB)
	$(CC) $(CFLAGS) -I"$(CURDIR)/$(STDLIB_DIR)" "$(CURDIR)/$(IMMUTABILITY_C_OUT)" "$(CURDIR)/$(STDLIB_LIB)" -o "$(CURDIR)/$(IMMUTABILITY_BIN)" $(IMMUTABILITY_LIBS)

$(IMMUTABILITY_PATHS_TEST_BIN): $(IMMUTABILITY_PATHS_TEST_C_OUT) $(STDLIB_LIB)
	$(CC) $(CFLAGS) -I"$(CURDIR)/$(STDLIB_DIR)" "$(CURDIR)/$(IMMUTABILITY_PATHS_TEST_C_OUT)" "$(CURDIR)/$(STDLIB_LIB)" -o "$(CURDIR)/$(IMMUTABILITY_PATHS_TEST_BIN)" $(IMMUTABILITY_LIBS)

unit: $(IMMUTABILITY_PATHS_TEST_BIN)
	./$(IMMUTABILITY_PATHS_TEST_BIN)

quick: $(BIN) unit
	@set +e; output="$$(NIXOS_CONFIG_ROOT="$(CURDIR)" ./$(BIN) 2>&1)"; status=$$?; \
	printf '%s\n' "$$output"; \
	test $$status -eq 1; \
	printf '%s\n' "$$output" | grep -q 'Usage: nixosctl'
	NIXOS_CONFIG_ROOT="$(CURDIR)" ./$(BIN) e2e tests/quick/test.json

smoke: quick

check test: quick stateful-host

host-smoke: quick

app-settings: $(BIN)
	$(MAKE) -C tests app-settings

stateful-host: $(BIN)
	$(MAKE) -C tests stateful-host

aarch64-qemu-host: $(BIN)
	$(MAKE) -C tests aarch64-qemu-host

x86_64-qemu-host: $(BIN)
	$(MAKE) -C tests x86_64-qemu-host

installer-download: $(BIN)
	$(MAKE) -C tests installer-download

install-system: $(BIN)
	$(MAKE) -C tests install-system

immutability-reset: $(BIN)
	$(MAKE) -C tests immutability-reset

immutability-key-encoding: $(BIN)
	$(MAKE) -C tests immutability-key-encoding

installer-ssh installer-ssh-smoke: $(BIN)
	$(MAKE) -C tests installer-ssh

tpm2-probe: $(BIN)
	$(MAKE) -C tests tpm2-probe

secure-boot-capabilities: $(BIN)
	$(MAKE) -C tests secure-boot-capabilities

secure-boot-install: $(BIN)
	$(MAKE) -C tests secure-boot-install

secure-boot-lanzaboote: $(BIN)
	$(MAKE) -C tests secure-boot-lanzaboote

graph-list: $(BIN)
	$(MAKE) -C tests graph-list

graph-status: $(BIN)
	$(MAKE) -C tests graph-status

graph-coverage: $(BIN)
	$(MAKE) -C tests graph-coverage

graph-early: $(BIN)
	$(MAKE) -C tests graph-early

graph-installer-ssh: $(BIN)
	$(MAKE) -C tests graph-installer-ssh

graph-full: $(BIN)
	$(MAKE) -C tests graph-full

chain: $(BIN)
	$(MAKE) -C tests chain

clean:
	rm -rf $(BUILD_DIR)
