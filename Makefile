# Use the compiler pinned in flake.lock by default. Override BTRC=... when
# intentionally testing a local btrcpy checkout.
BTRC ?= nix run --no-warn-dirty --inputs-from . btrc\#btrcpy --
CC ?= cc
CFLAGS ?= -std=c11 -pedantic
# Programs are transpiled program-only with strict imports and reference the
# prebuilt stdlib archive (built by `--build-stdlib`); they include
# btrc_stdlib.h and link libbtrc.a.
TRANSPILER_FLAGS ?= --no-stdlib --strict-imports --stdlib "$(CURDIR)/$(STDLIB_DIR)"
NIXOSCTL_LIBS ?= -lm -lpthread -lutil
IMMUTABILITY_LIBS ?= -lm -lpthread
TRAY_LIBS ?= $(shell pkg-config --cflags --libs dbus-1) -lm -lpthread

BUILD_DIR := build
STDLIB_DIR := $(BUILD_DIR)/stdlib
STDLIB_HEADER := $(STDLIB_DIR)/btrc_stdlib.h
STDLIB_IMPL := $(STDLIB_DIR)/btrc_stdlib.c
STDLIB_MANIFEST := $(STDLIB_DIR)/btrc_stdlib.manifest
STDLIB_LIB := $(BUILD_DIR)/libbtrc.a
BTRC_STAMP := $(BUILD_DIR)/.btrc-tool
NIXOSCTL_ENTRY := btrc/nixosctl/NixosCtl.btrc
IMMUTABILITY_ENTRY := btrc/immutability/Immutability.btrc
NIXOSCTL_C_OUT := $(BUILD_DIR)/nixosctl.c
IMMUTABILITY_C_OUT := $(BUILD_DIR)/immutability.c
NIXOSCTL_BIN := $(BUILD_DIR)/nixosctl
IMMUTABILITY_BIN := $(BUILD_DIR)/immutability
TRAY_ENTRY := btrc/nixosctl/NixosTray.btrc
TRAY_C_OUT := $(BUILD_DIR)/nixosctl-tray.c
TRAY_BIN := $(BUILD_DIR)/nixosctl-tray
# <binary name>:<entry point>, one per unit suite
UNIT_TESTS := \
	immutability_paths:tests/immutability/PathsTests.btrc \
	nixos_paths:tests/core/NixosPathsTests.btrc \
	nixos_config:tests/core/NixosConfigTests.btrc \
	passwd_driver:tests/core/PasswdDriverTests.btrc \
	secure_boot:tests/secure-boot/VerificationTests.btrc \
	spec:tests/harness/SpecTests.btrc \
	state_hash:tests/harness/StateHashTests.btrc
unit_name = $(firstword $(subst :, ,$(1)))
unit_entry = $(lastword $(subst :, ,$(1)))
UNIT_BINS := $(foreach t,$(UNIT_TESTS),$(BUILD_DIR)/$(call unit_name,$(t))_test)
BIN := $(NIXOSCTL_BIN)
BINS := $(NIXOSCTL_BIN) $(IMMUTABILITY_BIN) $(TRAY_BIN)
SOURCES := $(shell find btrc tests -name '*.btrc' | sort)

# Every one of these just forwards to tests/Makefile under the same name
TESTS_TARGETS := app-settings stateful-host qemu-host x86_64-qemu-host \
	aarch64-qemu-host installer-download download install-system \
	immutability-reset immutability-key-encoding installer-ssh password-change \
	tpm2-probe secure-boot-capabilities secure-boot-install secure-boot-lanzaboote \
	graph-list graph-status graph-coverage graph-installer-ssh chain clean-vms

.PHONY: all help dirs transpile build-stdlib build unit quick smoke host-smoke \
	check test gaming hdr-screencast install iso iso-write graph-required \
	graph-default graph-full graph-early installer-ssh-smoke clean $(TESTS_TARGETS)

all: build

help:
	@printf '%s\n' 'Build:'
	@printf '%s\n' '  build             nixosctl, immutability and the tray binary'
	@printf '%s\n' '  clean             remove build/'
	@printf '%s\n' ''
	@printf '%s\n' 'Tests, fastest first:'
	@printf '%s\n' '  unit              in-process suites, no VM (seconds)'
	@printf '%s\n' '  quick             unit + a CLI smoke + the graph node named "quick"'
	@printf '%s\n' '  check             quick + stateful-host + gaming; what to run before committing'
	@printf '%s\n' '  graph-default     the graph default set (alias: graph-early)'
	@printf '%s\n' '  graph-required    the graph required set, i.e. every guarded contract (alias: graph-full)'
	@printf '%s\n' '  hdr-screencast    KWin HDR capture; needs a real GPU, so not in check'
	@printf '%s\n' ''
	@printf '%s\n' 'Install:'
	@printf '%s\n' '  install           build the CLI and install this host from an ISO'
	@printf '%s\n' '  iso               build the installer ISO (ISO_SYSTEM=aarch64-linux for the Deck)'
	@printf '%s\n' '  iso-write         write it to DEVICE=/dev/sdX'
	@printf '%s\n' ''
	@printf '%s\n' 'One scenario at a time: make <node>, e.g. make immutability-reset'
	@printf '%s\n' '  make -C tests help   lists them; several run a real QEMU install'
	@printf '%s\n' ''
	@printf '%s\n' 'Neither graph-required nor graph-full runs every node: both mean the'
	@printf '%s\n' 'required set. Name a node explicitly for anything outside it.'

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

transpile: $(NIXOSCTL_C_OUT) $(IMMUTABILITY_C_OUT) $(TRAY_C_OUT)

$(NIXOSCTL_C_OUT): $(SOURCES) $(STDLIB_MANIFEST) $(BTRC_STAMP) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(NIXOSCTL_ENTRY)" -o "$(CURDIR)/$(NIXOSCTL_C_OUT)"

$(IMMUTABILITY_C_OUT): $(SOURCES) $(STDLIB_MANIFEST) $(BTRC_STAMP) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(IMMUTABILITY_ENTRY)" -o "$(CURDIR)/$(IMMUTABILITY_C_OUT)"

# Library.Tray's Linux provider binds libdbus, so its typed header import
# needs pkg-config at transpile time and dbus-1 at link time.
$(TRAY_C_OUT): $(SOURCES) $(STDLIB_MANIFEST) $(BTRC_STAMP) | dirs
	$(BTRC) $(TRANSPILER_FLAGS) "$(CURDIR)/$(TRAY_ENTRY)" -o "$(CURDIR)/$(TRAY_C_OUT)"

build: $(BINS)

$(NIXOSCTL_BIN): $(NIXOSCTL_C_OUT) $(STDLIB_LIB)
	$(CC) $(CFLAGS) -I"$(CURDIR)/$(STDLIB_DIR)" "$(CURDIR)/$(NIXOSCTL_C_OUT)" "$(CURDIR)/$(STDLIB_LIB)" -o "$(CURDIR)/$(NIXOSCTL_BIN)" $(NIXOSCTL_LIBS)

$(IMMUTABILITY_BIN): $(IMMUTABILITY_C_OUT) $(STDLIB_LIB)
	$(CC) $(CFLAGS) -I"$(CURDIR)/$(STDLIB_DIR)" "$(CURDIR)/$(IMMUTABILITY_C_OUT)" "$(CURDIR)/$(STDLIB_LIB)" -o "$(CURDIR)/$(IMMUTABILITY_BIN)" $(IMMUTABILITY_LIBS)

$(TRAY_BIN): $(TRAY_C_OUT) $(STDLIB_LIB)
	$(CC) $(CFLAGS) -I"$(CURDIR)/$(STDLIB_DIR)" "$(CURDIR)/$(TRAY_C_OUT)" "$(CURDIR)/$(STDLIB_LIB)" -o "$(CURDIR)/$(TRAY_BIN)" $(TRAY_LIBS)

define UNIT_TEST_RULES
$$(BUILD_DIR)/$(1)_test.c: $$(SOURCES) $$(STDLIB_MANIFEST) $$(BTRC_STAMP) | dirs
	$$(BTRC) $$(TRANSPILER_FLAGS) "$$(CURDIR)/$(2)" -o "$$@"

$$(BUILD_DIR)/$(1)_test: $$(BUILD_DIR)/$(1)_test.c $$(STDLIB_LIB)
	$$(CC) $$(CFLAGS) -I"$$(CURDIR)/$$(STDLIB_DIR)" "$$<" "$$(CURDIR)/$$(STDLIB_LIB)" -o "$$@" $$(NIXOSCTL_LIBS)
endef
$(foreach t,$(UNIT_TESTS),$(eval $(call UNIT_TEST_RULES,$(call unit_name,$(t)),$(call unit_entry,$(t)))))

unit: $(UNIT_BINS)
	@for test in $(UNIT_BINS); do ./$$test || exit 1; done

quick: $(BIN) unit
	@set +e; output="$$(NIXOS_CONFIG_ROOT="$(CURDIR)" ./$(BIN) 2>&1)"; status=$$?; \
	printf '%s\n' "$$output"; \
	test $$status -eq 1; \
	printf '%s\n' "$$output" | grep -q 'Usage: nixosctl'
	NIXOS_CONFIG_ROOT="$(CURDIR)" ./$(BIN) graph tests/graph.json run quick

smoke: quick

check test: quick stateful-host gaming

.PHONY: gaming
gaming:
	$(MAKE) -C tests gaming

# Needs a real GPU, so deliberately not part of check/test
.PHONY: hdr-screencast
hdr-screencast:
	nix/apps/gaming/sunshine/hdr-screencast-test/hdr-screencast-test $(ARGS)

# Installs this host from an installer ISO; nixosctl elevates and prompts itself
install: $(BIN)
	./$(BIN) install $(ARGS)

# ISO_SYSTEM builds for another architecture, e.g. make iso ISO_SYSTEM=aarch64-linux
ISO_SYSTEM ?= $(shell nix eval --raw --impure --expr builtins.currentSystem)
ISO_LINK := $(BUILD_DIR)/iso

iso: | dirs ## Build the installer ISO for ISO_SYSTEM -> build/iso
	nix build --no-warn-dirty ".#packages.$(ISO_SYSTEM).installer" -o "$(ISO_LINK)"
	@printf '%s\n' "$$(ls $(ISO_LINK)/iso/*.iso)"

iso-write: iso ## Write the ISO to DEVICE, e.g. make iso-write DEVICE=/dev/sdX
	@test -n "$(DEVICE)" || { printf '%s\n' 'set DEVICE, e.g. make iso-write DEVICE=/dev/sdX' >&2; exit 1; }
	@test -b "$(DEVICE)" || { printf '%s\n' '$(DEVICE) is not a block device' >&2; exit 1; }
	@lsblk -no NAME,SIZE,MODEL,TRAN,RM "$(DEVICE)"
	@printf '%s' 'erase this device? [y/N] ' && read answer && test "$$answer" = y
	sudo dd if="$$(ls $(ISO_LINK)/iso/*.iso)" of="$(DEVICE)" bs=4M status=progress oflag=sync

$(TESTS_TARGETS): $(BIN)
	$(MAKE) -C tests $@

# run-required covers the graph's required set, run with no targets covers default
graph-required: $(BIN)
	$(MAKE) -C tests graph-full

graph-default: $(BIN)
	$(MAKE) -C tests graph-early

# kept so existing invocations and muscle memory keep working
smoke host-smoke: quick
installer-ssh-smoke: installer-ssh
graph-full: graph-required
graph-early: graph-default

clean:
	rm -rf $(BUILD_DIR)
