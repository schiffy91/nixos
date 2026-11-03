NIX_SHELL ?= nix-shell
NIX_BUILD ?= nix-build
SHELL_NIX := tests/shell.nix
NIXOSCTL := build/nixosctl

.PHONY: help build quick host-smoke app-settings stateful-host qemu-host x86_64-qemu-host aarch64-qemu-host download installer-download install-system immutability-reset password-change immutability-key-encoding installer-ssh installer-ssh-smoke tpm2-probe secure-boot-capabilities secure-boot-install secure-boot-lanzaboote graph-list graph-status graph-coverage graph-early graph-installer-ssh graph-full chain clean-vms

help:
	@printf '%s\n' 'Run these from the repo root as `make <target>`; `make -C tests` also works.'
	@printf '%s\n' ''
	@printf '%s\n' 'Whole suites:'
	@printf '%s\n' '  quick             unit suites + a CLI smoke + the graph node named "quick"'
	@printf '%s\n' '  gaming            nix build of tests/gaming (proton, assetto)'
	@printf '%s\n' '  graph-early       the graph default set'
	@printf '%s\n' '  graph-full        the graph REQUIRED set, not every node'
	@printf '%s\n' '  graph-list        list nodes        graph-status   per-node state freshness'
	@printf '%s\n' '  graph-coverage    which spec operations the graph exercises'
	@printf '%s\n' ''
	@printf '%s\n' 'One scenario at a time:'
	@printf '%s\n' '  app-settings stateful-host qemu-host x86_64-qemu-host aarch64-qemu-host'
	@printf '%s\n' '  installer-download download installer-ssh graph-installer-ssh install-system'
	@printf '%s\n' '  immutability-reset immutability-key-encoding password-change tpm2-probe'
	@printf '%s\n' '  secure-boot-capabilities secure-boot-install secure-boot-lanzaboote chain'
	@printf '%s\n' ''
	@printf '%s\n' 'Any other node: build/nixosctl graph tests/graph.json run <node>'
	@printf '%s\n' '  clean-vms       delete recorded VM state under .vm/e2e'
	@printf '%s\n' ''
	@printf '%s\n' 'install-system, immutability-reset, password-change and chain run a real'
	@printf '%s\n' 'QEMU install, so they take minutes to tens of minutes each.'

build:
	$(MAKE) -C .. build

.PHONY: gaming
gaming:
	$(NIX_BUILD) gaming --no-out-link

quick: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run quick'

host-smoke: quick

app-settings: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run app-settings'

stateful-host: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run stateful-host --arg marker=make-target'

qemu-host: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run qemu-host'

x86_64-qemu-host: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run qemu-host-x86_64'

aarch64-qemu-host: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run qemu-host-aarch64'

download: installer-download

installer-download: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run installer-download'

install-system: installer-ssh
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run install-reset'

immutability-reset: install-system
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run immutability-reset'

password-change: install-system
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run password-change'

immutability-key-encoding: install-system
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run immutability-key-encoding'

installer-ssh: installer-download
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run installer-ssh'

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
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run-required'

chain: installer-download installer-ssh install-system immutability-reset

clean-vms:
	rm -rf ../.vm/e2e
