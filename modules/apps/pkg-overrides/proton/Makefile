SHELL := /usr/bin/env bash
.SHELLFLAGS := -euo pipefail -c
.DEFAULT_GOAL := help
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

NIXOS ?= /etc/nixos
PROTON_DIR ?= $(NIXOS)/modules/apps/pkg-overrides/proton
BNET_DEV ?= $(PROTON_DIR)/bin/bnet-dev

JOBS ?= $(shell nproc)
DELAY ?= 8
SECONDS ?= 3
OUTPUT ?= /tmp/bnet-dev-capture.png
LOG ?= /tmp/bnet-dev-capture.wrapper.log
PATCH_OUT ?= $(PROTON_DIR)/src/patches
RANGE ?= -1 HEAD

TARGETS ?= dlls/dcomp/all
ALL_WINE_TARGETS := \
	dlls/dcomp/all \
	dlls/dxgi/all \
	dlls/win32u/all \
	dlls/winevulkan/all \
	dlls/winewayland.drv/all \
	programs/explorer/all

ifdef SCWHINE_IN_DEV_SHELL
define bnet_run
$(1)
endef
else
define bnet_run
nix-shell "$(PROTON_DIR)/dev-shell.nix" --run '$(1)'
endef
endif

.PHONY: \
	help shell setup use restore status cleanup \
	setup-wine build build-all loop smoke capture shot \
	dcomp dxgi wayland explorer win32u winevulkan \
	overlay-wine overlay-dxvk \
	wine-status wine-diff wine-format-patch

help:
	@printf '%s\n' \
		'Battle.net Proton development loop' \
		'' \
		'Environment:' \
		'  make shell                 enter the Nix devshell' \
		'  make setup                 create dev compat tool and seed src/wine' \
		'  make status                show active compat tool and local source dirs' \
		'  make cleanup               kill Battle.net/Wine dev processes' \
		'' \
		'Build touched Wine targets:' \
		'  make dcomp                 build/overlay dlls/dcomp/all' \
		'  make wayland               build/overlay dlls/winewayland.drv/all' \
		'  make explorer              build/overlay programs/explorer/all' \
		'  make win32u                build/overlay dlls/win32u/all' \
		'  make dxgi                  build/overlay dlls/dxgi/all' \
		'  make build TARGETS="..."   build/overlay arbitrary Wine target list' \
		'' \
		'Run checks:' \
		'  make smoke SECONDS=5       launch briefly, log, then clean up' \
		'  make shot DELAY=8          launch, screenshot, then clean up' \
		'  make loop TARGETS="..."    build/overlay/smoke one target list' \
		'' \
		'Source patch workflow:' \
		'  make wine-status           show src/wine git status' \
		'  make wine-diff             show src/wine diff' \
		'  make wine-format-patch     export committed src/wine changes' \
		'' \
		'Common vars:' \
		'  JOBS=$(JOBS)' \
		'  DELAY=$(DELAY)' \
		'  OUTPUT=$(OUTPUT)' \
		'  RANGE=$(RANGE)' \
		'  PATCH_OUT=$(PATCH_OUT)'

shell:
	nix-shell "$(PROTON_DIR)/dev-shell.nix"

setup: use setup-wine

use:
	$(call bnet_run,$(BNET_DEV) setup)
	$(call bnet_run,$(BNET_DEV) use)

restore:
	$(call bnet_run,$(BNET_DEV) restore)

status:
	$(call bnet_run,$(BNET_DEV) status)

cleanup:
	$(call bnet_run,$(BNET_DEV) cleanup)

setup-wine:
	$(call bnet_run,$(BNET_DEV) setup-wine)

build:
	$(call bnet_run,$(BNET_DEV) build-wine --jobs "$(JOBS)" $(TARGETS))

build-all:
	$(call bnet_run,$(BNET_DEV) build-wine --jobs "$(JOBS)" $(ALL_WINE_TARGETS))

loop:
	$(call bnet_run,$(BNET_DEV) wine-loop --jobs "$(JOBS)" --seconds "$(SECONDS)" $(TARGETS))

smoke:
	$(call bnet_run,$(BNET_DEV) smoke --seconds "$(SECONDS)")

capture shot:
	$(call bnet_run,$(BNET_DEV) capture --delay "$(DELAY)" --output "$(OUTPUT)" --log "$(LOG)")

dcomp:
	$(MAKE) build TARGETS='dlls/dcomp/all'

dxgi:
	$(MAKE) build TARGETS='dlls/dxgi/all'

wayland:
	$(MAKE) build TARGETS='dlls/winewayland.drv/all'

explorer:
	$(MAKE) build TARGETS='programs/explorer/all'

win32u:
	$(MAKE) build TARGETS='dlls/win32u/all'

winevulkan:
	$(MAKE) build TARGETS='dlls/winevulkan/all'

overlay-wine:
	$(call bnet_run,$(BNET_DEV) overlay-wine --wine64 "$(PROTON_DIR)/src/wine64" --wine32 "$(PROTON_DIR)/src/wine32")

overlay-dxvk:
	$(call bnet_run,$(BNET_DEV) overlay-dxvk --dxvk "$(PROTON_DIR)/src/dxvk")

wine-status:
	git -C "$(PROTON_DIR)/src/wine" status --short --branch

wine-diff:
	git -C "$(PROTON_DIR)/src/wine" diff --stat
	git -C "$(PROTON_DIR)/src/wine" diff

wine-format-patch:
	mkdir -p "$(PATCH_OUT)"
	git -C "$(PROTON_DIR)/src/wine" format-patch -s -o "$(PATCH_OUT)" $(RANGE)
