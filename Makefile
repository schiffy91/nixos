# btrcpy and cc come from the dev shell (`nix develop`, or direnv via .envrc).
BTRC ?= btrcpy
PYTHON ?= python3
CC ?= cc
CFLAGS ?= -std=c11 -pedantic
TRANSPILER_FLAGS ?= --no-stdlib
NIXOSCTL_LIBS ?= -lm -lpthread -lutil
MEMBRANE_LIBS ?= -lm -lpthread

BUILD_DIR := build
GENERATED_DIR := generated
NIXOSCTL_ENTRY := bin/nixosctl.btrc
MEMBRANE_ENTRY := bin/semipermeable_membrane.btrc
NIXOSCTL_C_OUT := $(BUILD_DIR)/nixosctl.c
MEMBRANE_C_OUT := $(BUILD_DIR)/semipermeable_membrane.c
GENERATED_MEMBRANE_C := $(GENERATED_DIR)/semipermeable_membrane.c
GENERATED_NIXOSCTL_C := $(GENERATED_DIR)/nixosctl.c
NIXOSCTL_BIN := $(BUILD_DIR)/nixosctl
MEMBRANE_BIN := $(BUILD_DIR)/semipermeable_membrane
BIN := $(NIXOSCTL_BIN)
BINS := $(NIXOSCTL_BIN) $(MEMBRANE_BIN)
SOURCES := $(shell find bin lib tests/e2e vendor -name '*.btrc' | sort)

.PHONY: all transpile build generated check quick smoke test host-smoke stdlib-sync-check app-settings stateful-host aarch64-qemu-host installer-download install-system immutability-reset installer-ssh installer-ssh-smoke tpm2-probe secure-boot-capabilities secure-boot-install secure-boot-lanzaboote graph-list graph-status graph-coverage graph-early graph-installer-ssh graph-full chain clean dirs

all: build

dirs:
	mkdir -p $(BUILD_DIR)

transpile: $(NIXOSCTL_C_OUT) $(MEMBRANE_C_OUT)

$(NIXOSCTL_C_OUT): $(SOURCES) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(NIXOSCTL_ENTRY)" -o "$(CURDIR)/$(NIXOSCTL_C_OUT)"

$(MEMBRANE_C_OUT): $(SOURCES) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(MEMBRANE_ENTRY)" -o "$(CURDIR)/$(MEMBRANE_C_OUT)"

build: $(BINS)

$(NIXOSCTL_BIN): $(NIXOSCTL_C_OUT)
	$(CC) $(CFLAGS) "$(CURDIR)/$(NIXOSCTL_C_OUT)" -o "$(CURDIR)/$(NIXOSCTL_BIN)" $(NIXOSCTL_LIBS)

$(MEMBRANE_BIN): $(MEMBRANE_C_OUT)
	$(CC) $(CFLAGS) "$(CURDIR)/$(MEMBRANE_C_OUT)" -o "$(CURDIR)/$(MEMBRANE_BIN)" $(MEMBRANE_LIBS)

generated: $(MEMBRANE_C_OUT) $(NIXOSCTL_C_OUT)
	mkdir -p $(GENERATED_DIR)
	cp $(MEMBRANE_C_OUT) $(GENERATED_MEMBRANE_C)
	cp $(NIXOSCTL_C_OUT) $(GENERATED_NIXOSCTL_C)

quick: $(BIN)
	@set +e; output="$$(./$(BIN) 2>&1)"; status=$$?; \
	printf '%s\n' "$$output"; \
	test $$status -eq 1; \
	printf '%s\n' "$$output" | grep -q 'Usage: nixosctl'
	./$(BIN) e2e tests/quick/test.json

smoke: quick

check test: stdlib-sync-check quick stateful-host

host-smoke: quick

stdlib-sync-check:
	diff -qr --exclude=build vendor/btrc-stdlib "$$BTRC_SRC/src/stdlib"

app-settings: $(BIN)
	$(MAKE) -C tests app-settings

stateful-host: $(BIN)
	$(MAKE) -C tests stateful-host

aarch64-qemu-host: $(BIN)
	$(MAKE) -C tests aarch64-qemu-host

installer-download: $(BIN)
	$(MAKE) -C tests installer-download

install-system: $(BIN)
	$(MAKE) -C tests install-system

immutability-reset: $(BIN)
	$(MAKE) -C tests immutability-reset

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
	rm -f $(NIXOSCTL_C_OUT) $(MEMBRANE_C_OUT) $(BINS)
