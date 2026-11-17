NIX_SHELL ?= nix-shell
NIX_BUILD ?= nix-build
SHELL_NIX := tests/shell.nix
NIXOSCTL := build/nixosctl
# A graph run starts many processes and signals them. Inside its own process namespace nothing it signals
# can reach the session that started it, and nothing of it is left when it ends. Used where bwrap exists.
ISOLATE ?= $(if $(shell command -v bwrap 2>/dev/null),bwrap --unshare-pid --dev-bind / / --proc /proc --,)
# The other architecture's nodes: native on a host of that architecture, emulated and slow anywhere else.
AARCH64_NODES := installer-ssh-aarch64 install-reset-aarch64 recovery-boot-aarch64 recovery-secure-boot-aarch64 secure-boot-aarch64 installer-image-aarch64
# Longer forms of required nodes, for a run that is not waited on.
NIGHTLY_NODES := drive-convergence-seeds

.PHONY: help build download installer-download install-system immutability-reset password-change immutability-key-encoding installer-ssh installer-ssh-smoke tpm2-probe secure-boot-capabilities secure-boot-install secure-boot-lanzaboote graph-list graph-status graph-coverage graph-gc graph-aarch64 graph-nightly graph-early graph-installer-ssh graph-full chain clean-vms

help:
	@printf '%s\n' 'Run these from the repo root as `make <target>`; `make -C tests` also works.'
	@printf '%s\n' ''
	@printf '%s\n' 'Whole suites:'
	@printf '%s\n' '  gaming            nix build of tests/gaming (proton, assetto)'
	@printf '%s\n' '  graph-early       the graph default set'
	@printf '%s\n' '  graph-full        the graph REQUIRED set, not every node'
	@printf '%s\n' '  graph-aarch64     the aarch64 nodes (emulated unless the host is aarch64)'
	@printf '%s\n' '  graph-nightly     longer forms of required nodes'
	@printf '%s\n' '  graph-list        list nodes        graph-status   per-node state freshness'
	@printf '%s\n' '  graph-coverage    which spec operations the graph exercises'
	@printf '%s\n' '  graph-gc          delete VM state no current node references (GC_ARGS=--dry-run lists it)'
	@printf '%s\n' ''
	@printf '%s\n' 'One scenario at a time:'
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

download: installer-download

installer-download: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run installer-download'

install-system: installer-ssh
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run install-reset'

immutability-reset: install-system
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run immutability-reset'

password-change: install-system
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run password-change'

immutability-key-encoding: install-system
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run immutability-key-encoding'

installer-ssh: installer-download
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run installer-ssh'

installer-ssh-smoke: installer-ssh

tpm2-probe: installer-download
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run tpm2-probe'

secure-boot-capabilities: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run secure-boot-capabilities'

secure-boot-install: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run secure-boot-install'

secure-boot-lanzaboote: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run secure-boot-lanzaboote'

graph-list: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json list'

graph-status: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json status'

graph-coverage: build
	cd .. && $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json coverage'

# GC_ARGS=--dry-run only lists what would go
graph-gc: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json gc $(GC_ARGS)'

graph-early: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run'

graph-installer-ssh: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run installer-ssh'

graph-aarch64: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run $(AARCH64_NODES)'

graph-nightly: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run $(NIGHTLY_NODES)'

graph-full: build
	cd .. && $(ISOLATE) $(NIX_SHELL) $(SHELL_NIX) --run 'NIXOS_CONFIG_ROOT=$$PWD ./$(NIXOSCTL) graph tests/graph.json run-required'

chain: installer-download installer-ssh install-system immutability-reset

clean-vms:
	rm -rf ../.vm/e2e
