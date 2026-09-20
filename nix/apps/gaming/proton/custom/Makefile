SHELL := /usr/bin/env bash
.SHELLFLAGS := -euo pipefail -c
.DEFAULT_GOAL := help
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

NIXOS ?= /etc/nixos
PROTON_DIR ?= $(NIXOS)/nix/apps/gaming/proton/custom
PROTON_CUSTOM_DEV ?= $(PROTON_DIR)/bin/proton-custom-dev

JOBS ?= $(shell nproc)
PATCH_OUT ?= $(PROTON_DIR)/src/patches
RANGE ?= -1 HEAD
TARGETS ?= dlls/winewayland.drv/all
STORE ?=

ALL_WINE_TARGETS := \
	dlls/ntdll/all \
	dlls/win32u/all \
	dlls/winewayland.drv/all

ifdef PROTON_CUSTOM_IN_DEV_SHELL
define proton_custom_run
$(1)
endef
else
define proton_custom_run
PROTON_CUSTOM_QUIET_SHELL=1 nix-shell "$(PROTON_DIR)/dev-shell.nix" --run '$(1)'
endef
endif

.PHONY: \
	help shell setup use restore status cleanup \
	reset-dev install-store \
	package setup-wine build build-all \
	wayland win32u ntdll \
	overlay-wine check-patches patch-lint source-drift reset-source-drift \
	wine-status wine-diff wine-format-patch

help:
	@printf '%s\n' \
		'proton-custom development' \
		'' \
		'Bootstrap without host make:' \
		'  ./bin/proton-custom-make setup      run targets through the Nix dev shell' \
		'  ./bin/proton-custom-build changed   build changed Wine targets' \
		'  ./bin/proton-custom-install dev     point Steam compat tool at the dev tree' \
		'' \
		'Environment:' \
		'  make shell                    enter the Nix dev shell' \
		'  make setup                    create dev compat tool and seed sources' \
		'  make reset-dev                recreate dev compat tool from active store' \
		'  make install-store STORE=...  point compat symlink at a built store path' \
		'  make status                   show active compat tool and source dirs' \
		'  make check-patches            verify Wine patch application only' \
		'  make patch-lint               check patch mailbox metadata hygiene' \
		'  make source-drift             compare dev source with patched package source' \
		'  make reset-source-drift       restore dev source to patched package source' \
		'  make package                  build the Nix compat-tool package' \
		'  make cleanup                  kill dev Wine processes for this prefix' \
		'' \
		'Build touched Wine targets:' \
		'  make wayland                  build/overlay dlls/winewayland.drv/all' \
		'  make ntdll                    build/overlay dlls/ntdll/all' \
		'  make win32u                   build/overlay dlls/win32u/all' \
		'  make build TARGETS="..."      build/overlay arbitrary Wine targets' \
		'' \
		'Source patch workflow:' \
		'  make wine-status              show src/wine git status' \
		'  make wine-diff                show src/wine diff' \
		'  make wine-format-patch        export committed src/wine changes' \
		'' \
		'Common vars:' \
		'  JOBS=$(JOBS)' \
		'  RANGE=$(RANGE)' \
		'  PATCH_OUT=$(PATCH_OUT)'

shell:
	nix-shell "$(PROTON_DIR)/dev-shell.nix"

setup: use setup-wine

reset-dev:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) setup --force)
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) use)

install-store:
	@test -n "$(STORE)" || (echo 'STORE is required' >&2; exit 1)
	$(PROTON_DIR)/bin/proton-custom-install store "$(STORE)"

use:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) setup)
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) use)

restore:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) restore)

status:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) status)

check-patches:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) check-patches)


patch-lint:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) patch-lint)

source-drift:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) source-drift)

reset-source-drift:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) reset-source-drift)

package:
	nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file://$(NIXOS)"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in pkgs.callPackage $(PROTON_DIR)/package.nix { inherit (pkgs) makeWrapper rsync unzip; }'

cleanup:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) cleanup)

setup-wine:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) setup-wine)


build:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) build-wine --jobs "$(JOBS)" $(TARGETS))


build-all:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) build-wine --jobs "$(JOBS)" $(ALL_WINE_TARGETS))



wayland:
	$(MAKE) build TARGETS='dlls/winewayland.drv/all'


win32u:
	$(MAKE) build TARGETS='dlls/win32u/all'

ntdll:
	$(MAKE) build TARGETS='dlls/ntdll/all'




overlay-wine:
	$(call proton_custom_run,$(PROTON_CUSTOM_DEV) overlay-wine --wine64 "$(PROTON_DIR)/src/wine64" --wine32 "$(PROTON_DIR)/src/wine32")



wine-status:
	git -C "$(PROTON_DIR)/src/wine" status --short --branch

wine-diff:
	git -C "$(PROTON_DIR)/src/wine" diff --stat
	git -C "$(PROTON_DIR)/src/wine" diff

wine-format-patch:
	mkdir -p "$(PATCH_OUT)"
	git -C "$(PROTON_DIR)/src/wine" format-patch -s -o "$(PATCH_OUT)" $(RANGE)



