---
name: nixos-init
description: Load full context for the NixOS configuration at /etc/nixos. Use before answering questions about this repo or changing system, disk, boot, recovery, BTRC, or simulator behavior.
---

# nixos-init

- Treat /etc/nixos as the deployed copy of this repository.
- Read flake.nix, nix/settings.nix, nix/system, nix/hosts, btrc, and tests before changing behavior.
- Prefer existing module boundaries and settings patterns over new abstractions.
- For BTRC, use stdlib helpers, f-strings, structured parsers, and direct shell helpers where they make code simpler.
- Do not reintroduce live immutability baseline recapture during nixosctl update. Immutability recovery belongs in boot/initrd paths and simulator coverage.
- Verify disk, boot, recovery, and immutability changes with targeted Nix evals and simulator tests for x86_64 and aarch64 when feasible.

## Simulator (e2e) tests: use the graph runner, never hand-run specs

- One spec per scenario in `tests/<name>/test.json`; `tests/graph.json` wires nodes (`after` parents, per-node `args`, `default` and `required` sets). The harness is BTRC under `tests/e2e/` (spec parser, runner, qemu harness, `graph/runner.btrc`).
- Run: `make build && nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run <node ...>'` (shortcuts: `make -C tests graph-status|graph-early|graph-full`, or a node target like `make -C tests immutability-reset`). The runner orders parents first, keys every state on `{{sourceHash}}` (sha256 of source files + `build/nixosctl`), skips nodes whose recorded state is `ready`, reruns stale ones; `--arg force=true` reruns regardless. Arch-suffixed nodes resolve to the host arch; aarch64 runs under TCG and is slow.
- Do not run `nixosctl e2e tests/<x>/test.json` by hand: `{{sourceHash}}` stays unexpanded, so parent states match VMs installed from old source and the run proves nothing.
- Guest ops execute over `ssh root@localhost -p <sshPort>`; write negative checks as `if cmd; then exit 1; fi` (a bare `! pipeline` never trips `set -e`). VM states live in `.vm/e2e/<state>-<hash>`; the VM admin/LUKS password is `testpass123`; `/var/tmp` survives reboots inside a VM, `/tmp` and `/root` do not.
- New scenarios = spec + graph node; add to `required` when they guard a contract.
- `{{sourceHash}}` hashes every tracked source file and is available to any spec, but only folds into a node's state hash when that node's own `stateMaterial` template explicitly references `{{sourceHash}}` (`argsMaterial()` in `tests/e2e/spec/test_spec.btrc` skips it otherwise). Nodes that build from this repo (`install-*`) must declare it; nodes that don't (`installer-download`, `installer-ssh`) correctly stay untouched by unrelated edits — they also don't need to redeclare it for a source-derived parent's changes, since `parent=<parent's recorded hash>` is always part of the hash and cascades automatically through a real `graph run` (parents rerun and re-record before children are checked). `graphName`/`nodeId`/`workspaceRoot` are excluded from every node's hash unconditionally — pure plumbing, never state.

Read the repository file list first:

`find /etc/nixos -type f -not -path '*/.git/*' -not -path '*/.direnv/*' -not -path '*/__pycache__/*' -not -path '*/secrets/*' -not -name '*.pyc' -not -name '*.dll' -not -name '*.dll.so' -not -name '*.exe' -not -name '*.so' -not -name 'flake.lock' -not -name 'package-lock.json' | sort`
