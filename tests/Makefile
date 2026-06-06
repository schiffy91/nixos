NIX_SHELL ?= nix-shell
SHELL_NIX := tests/shell.nix
NIXOSCTL := build/nixosctl

.PHONY: help build quick host-smoke app-settings stateful-host aarch64-qemu-host download installer-download install-system immutability-reset installer-ssh installer-ssh-smoke tpm2-probe secure-boot-capabilities secure-boot-install secure-boot-lanzaboote graph-list graph-status graph-coverage graph-early graph-installer-ssh graph-full chain clean-vms

help:
	@printf '%s\n' 'VM test targets:'
	@printf '%s\n' '  make -C tests quick'
	@printf '%s\n' '  make -C tests app-settings'
	@printf '%s\n' '  make -C tests stateful-host'
	@printf '%s\n' '  make -C tests aarch64-qemu-host'
	@printf '%s\n' '  make -C tests installer-download'
	@printf '%s\n' '  make -C tests install-system'
	@printf '%s\n' '  make -C tests immutability-reset'
	@printf '%s\n' '  make -C tests installer-ssh'
	@printf '%s\n' '  make -C tests tpm2-probe'
	@printf '%s\n' '  make -C tests secure-boot-capabilities'
	@printf '%s\n' '  make -C tests secure-boot-lanzaboote'
	@printf '%s\n' '  make -C tests graph-early'
	@printf '%s\n' '  make -C tests graph-installer-ssh'
	@printf '%s\n' '  make -C tests graph-coverage'
	@printf '%s\n' '  make -C tests graph-full'
	@printf '%s\n' '  make -C tests chain'
	@printf '%s\n' ''
	@printf '%s\n' 'install-system, immutability-reset, and chain run a real QEMU install.'

build:
	$(MAKE) -C .. build

quick: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) e2e tests/quick/test.json'

host-smoke: quick

app-settings: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run app-settings'

stateful-host: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) e2e tests/stateful-host/test.json --arg marker=make-target'

aarch64-qemu-host: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run aarch64-qemu-host'

download: installer-download

installer-download: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) e2e tests/installer-download/test.json'

install-system: installer-ssh
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) e2e tests/install-system/test.json'

immutability-reset: install-system
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) e2e tests/immutability-reset/test.json'

installer-ssh: installer-download
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) e2e tests/installer-ssh/test.json'

installer-ssh-smoke: installer-ssh

tpm2-probe: installer-download
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run tpm2-probe'

secure-boot-capabilities: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run secure-boot-capabilities'

secure-boot-install: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run secure-boot-install'

secure-boot-lanzaboote: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run secure-boot-lanzaboote'

graph-list: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json list'

graph-status: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json status'

graph-coverage: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json coverage'

graph-early: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run'

graph-installer-ssh: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run installer-ssh'

graph-full: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run cli-parity qmp-probe tpm2-probe immutability-reset install-disabled'

chain: installer-download installer-ssh install-system immutability-reset

clean-vms:
	rm -rf ../.vm/e2e
