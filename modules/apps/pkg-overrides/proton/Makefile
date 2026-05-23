SHELL := /usr/bin/env bash
.SHELLFLAGS := -euo pipefail -c
.DEFAULT_GOAL := help
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

NIXOS ?= /etc/nixos
PROTON_DIR ?= $(NIXOS)/modules/apps/pkg-overrides/proton
BNET_DEV ?= $(PROTON_DIR)/bin/bnet-dev
BNET_MAKE ?= $(PROTON_DIR)/bin/bnet-make

JOBS ?= $(shell nproc)
DELAY ?= 8
SECONDS ?= 3
LAUNCH_SECONDS ?= 0
OUTPUT ?= /tmp/bnet-dev-capture.png
OUTPUT_DIR ?= /tmp/bnet-dev-frames
LOG ?= /tmp/bnet-dev-capture.wrapper.log
COUNT ?= 24
INTERVAL ?= 0.15
FOCUS ?= 1
MODE ?= full
FORMAT ?= jpg
PROFILE ?= battle-net
TRACE ?= quiet
WINEDEBUG ?=
PATCH_OUT ?= $(PROTON_DIR)/src/patches
RANGE ?= -1 HEAD

TARGETS ?= dlls/dcomp/all
ALL_WINE_TARGETS := \
	dlls/dcomp/all \
	dlls/dxgi/all \
	dlls/ntdll/all \
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
SCWHINE_QUIET_SHELL=1 nix-shell "$(PROTON_DIR)/dev-shell.nix" --run '$(1)'
endef
endif

.PHONY: \
	help shell setup use restore status cleanup \
	setup-wine setup-dxvk build build-dxvk build-all loop smoke capture shot frames \
	launch trace focus grab-frames analyze-frames \
	hover-frames \
	dcomp dxgi wayland explorer win32u winevulkan dxvk \
	ntdll \
	overlay-wine overlay-dxvk restore-dxvk check-patches \
	wine-status wine-diff wine-format-patch \
	dxvk-status dxvk-diff dxvk-format-patch

help:
	@printf '%s\n' \
		'Battle.net Proton development loop' \
		'' \
		'Bootstrap without host make:' \
		'  ./bin/bnet-make setup     run targets through the Nix devshell' \
		'' \
		'Environment:' \
		'  make shell                 enter the Nix devshell' \
		'  make setup                 create dev compat tool and seed local sources' \
		'  make setup-dxvk            seed/configure src/dxvk' \
		'  make status                show active compat tool and local source dirs' \
		'  make check-patches         verify Wine/DXVK patch application only' \
		'  make cleanup               kill Battle.net/Wine dev processes' \
		'' \
		'Build touched Wine targets:' \
		'  make dcomp                 build/overlay dlls/dcomp/all' \
		'  make wayland               build/overlay dlls/winewayland.drv/all' \
		'  make explorer              build/overlay programs/explorer/all' \
		'  make ntdll                 build/overlay dlls/ntdll/all' \
		'  make win32u                build/overlay dlls/win32u/all' \
		'  make dxgi                  build/overlay dlls/dxgi/all' \
		'  make build TARGETS="..."   build/overlay arbitrary Wine target list' \
		'  make dxvk                  build/overlay DXVK dxgi.dll and d3d11.dll' \
		'  make restore-dxvk          overlay the Nix-built DXVK into the dev tool' \
		'' \
		'Run checks:' \
		'  make launch TRACE=quiet    launch and leave running for manual repro' \
		'  make trace                 launch with DComp/DXVK composition logs' \
		'  make smoke SECONDS=5       launch briefly, log, then clean up' \
		'  make shot DELAY=8          launch, screenshot, then clean up' \
		'  make frames DELAY=8        launch, capture a burst of frames, then clean up' \
		'  make focus                 raise the Battle.net window with KWin' \
		'  make grab-frames           capture a burst from an already-running session' \
		'  make hover-frames          move the pointer across Battle.net while capturing frames' \
		'  make analyze-frames        print consecutive frame differences' \
		'  make loop TARGETS="..."    build/overlay/smoke one target list' \
		'' \
		'Source patch workflow:' \
		'  make wine-status           show src/wine git status' \
		'  make wine-diff             show src/wine diff' \
		'  make wine-format-patch     export committed src/wine changes' \
		'  make dxvk-status           show src/dxvk git status' \
		'  make dxvk-diff             show src/dxvk diff' \
		'  make dxvk-format-patch     export committed src/dxvk changes' \
		'' \
		'Common vars:' \
		'  JOBS=$(JOBS)' \
		'  DELAY=$(DELAY)' \
		'  LAUNCH_SECONDS=$(LAUNCH_SECONDS)' \
		'  OUTPUT=$(OUTPUT)' \
		'  OUTPUT_DIR=$(OUTPUT_DIR)' \
		'  RANGE=$(RANGE)' \
		'  PATCH_OUT=$(PATCH_OUT)'

shell:
	nix-shell "$(PROTON_DIR)/dev-shell.nix"

setup: use setup-wine setup-dxvk

use:
	$(call bnet_run,$(BNET_DEV) setup)
	$(call bnet_run,$(BNET_DEV) use)

restore:
	$(call bnet_run,$(BNET_DEV) restore)

status:
	$(call bnet_run,$(BNET_DEV) status)

check-patches:
	$(call bnet_run,$(BNET_DEV) check-patches)

cleanup:
	$(call bnet_run,$(BNET_DEV) cleanup)

setup-wine:
	$(call bnet_run,$(BNET_DEV) setup-wine)

setup-dxvk:
	$(call bnet_run,$(BNET_DEV) setup-dxvk)

build:
	$(call bnet_run,$(BNET_DEV) build-wine --jobs "$(JOBS)" $(TARGETS))

build-dxvk:
	$(call bnet_run,$(BNET_DEV) build-dxvk --jobs "$(JOBS)")

build-all:
	$(call bnet_run,$(BNET_DEV) build-wine --jobs "$(JOBS)" $(ALL_WINE_TARGETS))

loop:
	$(call bnet_run,$(BNET_DEV) wine-loop --jobs "$(JOBS)" --seconds "$(SECONDS)" $(TARGETS))

smoke:
	$(call bnet_run,$(BNET_DEV) smoke --seconds "$(SECONDS)")

launch:
	$(call bnet_run,$(BNET_DEV) launch --trace "$(TRACE)" --seconds "$(LAUNCH_SECONDS)" --log "$(LOG)" $(if $(WINEDEBUG),--winedebug '$(WINEDEBUG)',))

trace:
	$(call bnet_run,$(BNET_DEV) launch --trace composition --seconds "$(LAUNCH_SECONDS)" --log "$(LOG)")

capture shot:
	$(call bnet_run,$(BNET_DEV) capture --delay "$(DELAY)" --output "$(OUTPUT)" --log "$(LOG)")

focus:
	$(call bnet_run,$(BNET_DEV) focus)

frames:
	$(call bnet_run,$(BNET_DEV) frames --delay "$(DELAY)" --count "$(COUNT)" --interval "$(INTERVAL)" --output-dir "$(OUTPUT_DIR)" --log "$(LOG)")

grab-frames:
	$(call bnet_run,$(BNET_DEV) grab-frames --count "$(COUNT)" --interval "$(INTERVAL)" --mode "$(MODE)" --format "$(FORMAT)" --output-dir "$(OUTPUT_DIR)" $(if $(filter 1 true yes,$(FOCUS)),--focus,))

hover-frames:
	$(call bnet_run,$(BNET_DEV) hover-frames --count "$(COUNT)" --interval "$(INTERVAL)" --mode "$(MODE)" --format "$(FORMAT)" --profile "$(PROFILE)" --output-dir "$(OUTPUT_DIR)")

analyze-frames:
	$(call bnet_run,$(BNET_DEV) analyze-frames --output-dir "$(OUTPUT_DIR)")

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

ntdll:
	$(MAKE) build TARGETS='dlls/ntdll/all'

winevulkan:
	$(MAKE) build TARGETS='dlls/winevulkan/all'

dxvk:
	$(MAKE) build-dxvk

overlay-wine:
	$(call bnet_run,$(BNET_DEV) overlay-wine --wine64 "$(PROTON_DIR)/src/wine64" --wine32 "$(PROTON_DIR)/src/wine32")

overlay-dxvk:
	$(call bnet_run,$(BNET_DEV) overlay-dxvk --dxvk "$(PROTON_DIR)/src/dxvk")

restore-dxvk:
	$(call bnet_run,$(BNET_DEV) restore-dxvk)

wine-status:
	git -C "$(PROTON_DIR)/src/wine" status --short --branch

wine-diff:
	git -C "$(PROTON_DIR)/src/wine" diff --stat
	git -C "$(PROTON_DIR)/src/wine" diff

wine-format-patch:
	mkdir -p "$(PATCH_OUT)"
	git -C "$(PROTON_DIR)/src/wine" format-patch -s -o "$(PATCH_OUT)" $(RANGE)

dxvk-status:
	git -C "$(PROTON_DIR)/src/dxvk" status --short --branch

dxvk-diff:
	git -C "$(PROTON_DIR)/src/dxvk" diff --stat
	git -C "$(PROTON_DIR)/src/dxvk" diff

dxvk-format-patch:
	mkdir -p "$(PATCH_OUT)"
	git -C "$(PROTON_DIR)/src/dxvk" format-patch -s -o "$(PATCH_OUT)" $(RANGE)
