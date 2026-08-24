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
- `{{sourceHash}}` hashes every tracked source file and is folded into every node's state hash regardless of that node's own `stateMaterial` template — editing any `.btrc`/`.nix`/`.json`/etc. anywhere invalidates the WHOLE chain (installer-download included), not just the node you touched. A from-scratch graph run after any edit is expected, not a bug.

Read the repository file list first:

`find /etc/nixos -type f -not -path '*/.git/*' -not -path '*/.direnv/*' -not -path '*/__pycache__/*' -not -path '*/secrets/*' -not -name '*.pyc' -not -name '*.dll' -not -name '*.dll.so' -not -name '*.exe' -not -name '*.so' -not -name 'flake.lock' -not -name 'package-lock.json' | sort`
