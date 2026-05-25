SHELL := /usr/bin/env bash
.SHELLFLAGS := -euo pipefail -c
.DEFAULT_GOAL := help
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

NIXOS ?= /etc/nixos
BATTLENET_DIR ?= $(NIXOS)/modules/apps/battlenet
PROTON_DIR ?= $(NIXOS)/modules/apps/pkg-overrides/proton-custom

BUILD_MODE ?= changed
REPEAT ?= 3
DELAY ?= 20
LOG_LINES ?= 120
FRAMES ?= 4
FRAME_INTERVAL ?= 1
CAPTURE_FALLBACK ?= foreground
ARTIFACT_DIR ?=
STOP_ON_WHITE ?= 0
COUNT ?= 24
INTERVAL ?= 0.12
OUTPUT ?= /tmp/battlenet-window.png
OUTPUT_DIR ?= /tmp/battlenet-window-frames

ifdef BATTLENET_IN_DEV_SHELL
define battlenet_run
$(1)
endef
else
define battlenet_run
BATTLENET_QUIET_SHELL=1 nix-shell "$(BATTLENET_DIR)/dev-shell.nix" --run '$(1)'
endef
endif

.PHONY: \
	help shell build install configure run cycle cycle-no-run \
	check build-capture-helper status logs diagnose sni context-menu install-capture-helper probe-window reproduce capture-window frames-window \
	focus cleanup proton-status proton-restore

help:
	@printf '%s\n' \
		'Battle.net development and repro loop' \
		'' \
		'Primary loop:' \
		'  make cycle BUILD_MODE=changed      build proton-custom, install dev tool, configure, run, diagnose' \
		'  make cycle-no-run                 build/install/configure without launching Battle.net' \
		'  make run                          run the same battlenet command KDE uses' \
		'' \
		'Single steps:' \
		'  make build                        build changed proton-custom targets' \
		'  make install                      point Steam compat tool at the dev proton-custom tree' \
		'  make configure                    prepare Battle.net prefix for current proton-custom' \
		'  make cleanup                      stop this Battle.net/Wine prefix' \
		'  make check                        syntax-check the Battle.net scripts' \
		'  make build-capture-helper         build the KWin by-window capture helper' \
		'' \
		'Debug:' \
		'  make status                       print compat, process, desktop-entry, and log state' \
		'  make logs LOG_LINES=180           print redacted logs' \
		'  make diagnose                     status + SNI metadata + redacted logs' \
		'  make sni                          print StatusNotifierItem metadata' \
		'  make context-menu                 call the tray ContextMenu method without capture' \
		'  make install-capture-helper       install local KWin screenshot-authorized helper' \
		'  make probe-window                 classify Battle.net window pixels, delete temp capture' \
		'  make reproduce                    repeat KDE desktop-entry launch + screenshot artifacts' \
		'  make capture-window               active Battle.net window only; never full desktop' \
		'  make frames-window                active-window frame burst for flicker analysis' \
		'' \
		'Common vars:' \
		'  BUILD_MODE=$(BUILD_MODE)' \
		'  REPEAT=$(REPEAT)' \
		'  DELAY=$(DELAY)' \
		'  LOG_LINES=$(LOG_LINES)' \
		'  FRAMES=$(FRAMES)' \
		'  FRAME_INTERVAL=$(FRAME_INTERVAL)' \
		'  CAPTURE_FALLBACK=$(CAPTURE_FALLBACK)' \
		'  ARTIFACT_DIR=$(ARTIFACT_DIR)' \
		'  STOP_ON_WHITE=$(STOP_ON_WHITE)' \
		'  OUTPUT=$(OUTPUT)' \
		'  OUTPUT_DIR=$(OUTPUT_DIR)'

shell:
	nix-shell "$(BATTLENET_DIR)/dev-shell.nix"

build:
	$(PROTON_DIR)/bin/proton-custom-build "$(BUILD_MODE)"

install:
	$(PROTON_DIR)/bin/proton-custom-install dev

configure:
	$(BATTLENET_DIR)/bin/battlenet-configure

check:
	bash -n \
		$(BATTLENET_DIR)/bin/battlenet-configure \
		$(BATTLENET_DIR)/bin/battlenet-cycle \
		$(BATTLENET_DIR)/bin/battlenet-debug \
		$(BATTLENET_DIR)/bin/battlenet-make \
		$(BATTLENET_DIR)/bin/battlenet-reproduce \
		$(BATTLENET_DIR)/bin/battlenet-run

build-capture-helper:
	$(call battlenet_run,mkdir -p $(BATTLENET_DIR)/.build && cc $(BATTLENET_DIR)/battlenet-capture-window.c -o $(BATTLENET_DIR)/.build/battlenet-capture-window $$(pkg-config --cflags --libs gio-2.0 gio-unix-2.0 glib-2.0))

run:
	$(BATTLENET_DIR)/bin/battlenet-run

cycle:
	DELAY="$(DELAY)" LOG_LINES="$(LOG_LINES)" $(BATTLENET_DIR)/bin/battlenet-cycle "$(BUILD_MODE)"

cycle-no-run:
	$(BATTLENET_DIR)/bin/battlenet-cycle "$(BUILD_MODE)" --no-run

status:
	$(BATTLENET_DIR)/bin/battlenet-debug status

logs:
	$(BATTLENET_DIR)/bin/battlenet-debug logs "$(LOG_LINES)"

diagnose:
	$(BATTLENET_DIR)/bin/battlenet-debug diagnose "$(LOG_LINES)"

sni:
	$(BATTLENET_DIR)/bin/battlenet-debug sni

context-menu:
	$(BATTLENET_DIR)/bin/battlenet-debug context-menu

install-capture-helper:
	$(call battlenet_run,$(BATTLENET_DIR)/bin/battlenet-debug install-capture-helper)

probe-window:
	$(call battlenet_run,$(BATTLENET_DIR)/bin/battlenet-debug probe-window)

reproduce:
	$(call battlenet_run,$(BATTLENET_DIR)/bin/battlenet-reproduce --repeat "$(REPEAT)" --delay "$(DELAY)" --log-lines "$(LOG_LINES)" --frames "$(FRAMES)" --frame-interval "$(FRAME_INTERVAL)" --capture-fallback "$(CAPTURE_FALLBACK)" --artifact-dir "$(ARTIFACT_DIR)" --stop-on-white "$(STOP_ON_WHITE)")

capture-window:
	$(call battlenet_run,$(BATTLENET_DIR)/bin/battlenet-debug capture-window "$(OUTPUT)")

frames-window:
	$(call battlenet_run,COUNT="$(COUNT)" INTERVAL="$(INTERVAL)" $(BATTLENET_DIR)/bin/battlenet-debug frames-window "$(OUTPUT_DIR)")

focus:
	$(PROTON_DIR)/bin/proton-custom-dev focus

cleanup:
	$(PROTON_DIR)/bin/proton-custom-dev cleanup

proton-status:
	$(PROTON_DIR)/bin/proton-custom-install status

proton-restore:
	$(PROTON_DIR)/bin/proton-custom-install restore
