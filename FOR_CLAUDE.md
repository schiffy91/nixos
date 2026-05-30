# FOR CLAUDE CODE

This file is a handoff for future Claude Code sessions working on this NixOS
repository.

## Current Intent

System management is **BTRC plus Nix**. There is no Python and no Rust in the
management tooling anymore — the only Python left in the tree is an x86-gated
package-override helper (`modules/apps/pkg-overrides/sunshine/edid/generate.py`)
that is upstream packaging, not management logic.

The user has been explicit about these preferences:

- Do not ask for permission unless there is a real blocker.
- Do not stop at a plan when implementation is possible.
- Use BTRC idioms in BTRC code (classes, small managers, `Command(...)` chains,
  `UnixShell`, stdlib wrappers) instead of C pasted into BTRC.
- Prefer adding stdlib wrappers over dropping into C-style code at call sites.
- Use the declarative, graph-based VM e2e harness for system behavior.
- Test expected behavior, not negative assertions for things never implemented.
- Do not remove NixOS desktop/application configuration casually.
- Gate x86-specific things narrowly and preserve host customizations.
- VFIO is out of scope and should not be reintroduced without a fresh design.

## Repository Overview

The BTRC management tree lives at the **repository root** (there is no `btrc/`
subdirectory and no `scripts/` tree).

| Path | Meaning |
|---|---|
| `flake.nix` | Builds every host and boot target; pins the BTRC compiler input; defines `default` + `btrc-build` dev shells |
| `bin/` | BTRC entrypoints: `nixosctl.btrc` (the management CLI) and `semipermeable_membrane.btrc` (immutability) |
| `lib/` | BTRC libraries (e.g. `lib/btrfs/`, `lib/immutability/`) |
| `generated/` | Checked-in transpiled C the Nix modules compile (`nixosctl.c`, `semipermeable_membrane.c`) |
| `vendor/btrc-stdlib/` | Vendored BTRC stdlib used by the `--no-stdlib` build |
| `Makefile` | Root build/test contract (`make build`, `make check`, `make -C tests graph-*`) |
| `modules/settings.nix` | Stable settings API |
| `modules/system/` | Shared system modules (incl. `immutability.nix`) |
| `modules/apps/` | Settings-gated app modules (incl. `nixosctl.nix`, `helper.nix`) |
| `modules/hosts/x86_64/FRACTAL-NORTH/` | Main workstation |
| `modules/hosts/aarch64/QEMU/` | ARM QEMU test host |
| `tests/` | Declarative graph VM e2e harness (`graph.json` + per-scenario `test.json`) |
| `README.md` | Main detailed docs |

Host discovery convention: `modules/hosts/<arch>/<HOST>/<HOST>.nix`. The aarch64
host is intentionally uppercase `QEMU`, matching the `FRACTAL-NORTH` style.

The BTRC compiler is an explicit flake input, pinned in `flake.lock`:

```nix
inputs.btrc.url = "github:schiffy91/btrc";
```

Local development can iterate against the sibling `~/Drive/dev/btrc` checkout;
push the branch, then `nix flake update btrc` to re-pin. The relative
`git+file:../btrc` form was tested and rejected (current Nix can mis-fetch it
during eval); the GitHub URL avoids that.

## BTRC ↔ Nix wiring

Two BTRC entrypoints transpile to checked-in C that the Nix modules compile:

- `bin/nixosctl.btrc` → `generated/nixosctl.c` → `modules/apps/nixosctl.nix`
  builds the `nixosctl` binary (`$CC -std=c11 -O2 ... -lm -lpthread -lutil`),
  `wrapProgram`s it with runtime tools on PATH, and adds it to
  `environment.systemPackages` under
  `lib.mkIf (settings.apps.enable && settings.apps.nixosHelper.enable)`.
- `bin/semipermeable_membrane.btrc` → `generated/semipermeable_membrane.c` →
  `modules/system/immutability.nix` builds the `semipermeable_membrane` binary
  used by the initrd reset service and the mounts service.

The generated C files are **checked in** because Nix needs them during system
evaluation; the compiler itself is packaged through the flake input. After
editing a `.btrc` entrypoint, run `make generated` and commit the regenerated C.
New `.nix`/`.c` files are invisible to `git+file` flake evaluation until staged
or committed — if a module silently fails to merge, check that its files are
tracked.

## App Settings

Optional app policy is controlled through `settings.apps.*` in
`modules/settings.nix` (all default `true`). `settings.apps.nixosHelper.enable`
gates the packaged `nixosctl` CLI. Battle.net, Rocksmith, and the Sunshine
overlay are additionally x86_64-gated. Do not solve app portability by disabling
the whole desktop stack — gate only the specific unsupported app/override.

`settings.desktop.enable` (default `true`) gates the Plasma6/SDDM desktop. The
headless e2e VM sets it `false` via the harness `desktop=none` arg; real hosts
keep `true`. This is an opt-out, not a removal.

## Immutability (semipermeable membrane)

Pure BTRC. `bin/semipermeable_membrane.btrc` implements:

- **Reset** in initrd: each `resetOnBoot` subvolume is rolled back to its read-
  only `CLEAN` snapshot; `persist.paths` are preserved into `@persist/dirs/<key>`
  subvolumes and mounted back by the `semipermeable-membrane-mounts` service.
- **Modes**: `reset`, `snapshot-only`, `restore-previous` (slot A) /
  `restore-penultimate` (slot B) / `restore-a`/`-b`/`-c`, `disabled`.
- **Orphan cleanup**: on a reconciling reset, `@persist/dirs/<key>` subvolumes
  whose key is no longer in the current spec are deleted (deepest-first).
- **`enforce.onReboot`** wires the initrd reset + mounts service.
- **`enforce.onUpdate`** adds a `snapshot-clean` activation script that
  re-captures each reset volume's `CLEAN` baseline at `nixos-rebuild switch`, so
  future factory-resets revert to the updated generation, not the install image.

BTRC gotcha worth remembering: `for x in self.method()` re-evaluates the call on
every iteration. Snapshot a list-returning call into a local `Vector` before a
loop that mutates what the call observes (deleting btrfs subvolumes inside such
a loop previously segfaulted on the shrinking vector and leaked the old root
subvolume each boot). `Set<string>()` in expression position is not accepted —
use a `Vector` + linear search.

## Live helper / tray

`modules/apps/helper.nix` no longer uses Python. The CLI is the packaged
`nixosctl`. The native system tray (macOS menu bar via NSStatusItem; Linux
Wayland StatusNotifierItem) is implemented in the BTRC stdlib in the sibling
`../btrc` repo (branch `claude/btrc-gui-tooling`: `src/stdlib/tray/`). Wiring it
into `helper.nix` is pending publishing that branch to `github:schiffy91/btrc`
and bumping this repo's `flake.lock` — see TODO below. A `# TODO` marks the spot.

## E2E VM Framework

The harness is the `nixosctl` binary itself (`nixosctl graph ...` / `e2e ...`),
driven from `tests/`.

```bash
make build                      # build nixosctl + semipermeable_membrane
make -C tests graph-list
make -C tests graph-coverage
make -C tests graph-full
make -C tests immutability-reset   # real QEMU install + reset
```

Graph file: `tests/graph.json` (nodes) + `tests/<scenario>/test.json` (specs).
Run a node: `nix-shell tests/shell.nix --run './build/nixosctl graph tests/graph.json run <node>'`.

Important nodes:

| Node | Purpose |
|---|---|
| `install-reset` / `install-disabled` | install reset-mode / disabled immutability |
| `immutability-reset` | boot installed disk; ephemeral wiped, persist kept |
| `immutability-disabled` | nothing is wiped |
| `immutability-snapshot-only` | ephemeral survives rotation |
| `immutability-restore` | restore-previous (A) / restore-penultimate (B) |
| `immutability-orphan` | dropped-key `@persist/dirs/<key>` subvolume is deleted |
| `immutability-files` | many small + a few big files across a reset |
| `immutability-onupdate` | `enforce.onUpdate` re-bases CLEAN at switch; marker survives reset |
| `nixosctl-cli` | packaged `nixosctl` CLI surface in-guest |
| `nixosctl-diff` | `nixosctl diff` in-guest |
| `secure-boot-*` | x86_64 Lanzaboote capability/install/verify |

The headless test VM is selected by `settings.desktop.enable = false` (harness
`desktop=none`). `configureVmHost` strips `modules/apps/*.nix` except
`nixosctl.nix`, so the packaged CLI is present for the cli/diff scenarios.

State files live under `.vm/e2e/` (symlinked to local scratch off the Google
Drive mount to avoid FUSE staleness). The workspace `sourceHash` includes
`*.btrc/*.nix/*.json/*.c`, so changing tracked sources forces a fresh install.

## Validation Commands

```bash
make build
make check                      # stdlib-sync-check + quick + stateful-host
make -C tests graph-coverage
make -C tests app-settings
make -C tests aarch64-qemu-host
nix flake check --all-systems --no-build --show-trace
```

`statix` still has low-value style warnings in dense settings expressions. Fix
warnings near code you are already touching; do not churn the whole settings
file just to satisfy style output.

## Known Caveats

- `Disk-Operation` targets are disko entrypoints, not installed boot targets.
  They use `modules/system/disk-operation.nix` so `nix flake check` can evaluate
  every `nixosConfiguration`.
- aarch64 Secure Boot: the Lanzaboote install + signing + key-enrollment
  staging are validated on aarch64 under HVF (`secure-boot-aarch64` scenario —
  installs, boots, `sbctl verify` → signed, `secure-boot-enroll force` staged).
  **Secure-boot firmware for aarch64 QEMU exists** — Debian's
  `AAVMF_CODE.secboot.fd` (and nixpkgs `OVMF.override{secureBoot=true}` builds an
  AAVMF too); the firmware was never the blocker. Runtime *enforcement*
  (`bootctl: Secure Boot: enabled`) needs QEMU `-machine virt,secure=on` (ARM
  TrustZone/EL3) + that enforcing secboot AAVMF. Two obstacles to validating
  enforcement on this Apple Silicon Mac, both real: (1) HVF "does not support
  providing Security extensions (TrustZone) to the guest CPU", so it needs
  `-accel tcg` (software emulation); and (2) under TCG the secboot AAVMF +
  secure world hangs at firmware init in this QEMU setup (a deep QEMU/EDK2
  secure-world issue, not chased further). Validating enforcement needs a
  TrustZone-capable aarch64 host. Harness gotchas found while chasing this:
  secure boot keys off the `secureBoot` **arg** (`argEnabled`), which is
  distinct from `bootTarget=Secure-Boot`; and `requireSecureBootCapability` +
  `secureFirmwareCodePath` previously hard-gated x86_64. x86_64 enforcement uses
  QEMU's bundled `edk2-x86_64-secure-code.fd` and works.
- The Google Drive (CloudStorage/FUSE) mount can serve stale git reads. Prefer
  non-worktree agents on the live tree; if you must use a worktree, verify it
  forked off the live `HEAD`, not a stale ref.
- The user may have an unrelated host QEMU process (SVM.app, port 2222). Do not
  kill host QEMU processes unless you can identify them as this repo's harness
  (the harness VMs use ports 2224+ and `.vm/e2e/...` paths).
- Do not commit `modules/hosts/x86_64/FRACTAL-NORTH/audio.nix` — it carries the
  user's in-progress local changes.

## Immediate TODOs

1. Integrate the native BTRC system tray into `modules/apps/helper.nix`.
   - Publish `../btrc` branch `claude/btrc-gui-tooling` to `github:schiffy91/btrc`.
   - `nix flake update btrc` here to re-pin `flake.lock`.
   - Replace the `helper.nix` tray `# TODO` with the BTRC tray daemon.

2. Re-sign and push commits once the 1Password SSH signer is available again
   (recent commits were made with `commit.gpgsign=false` while it was down).

3. Add true line/path coverage for BTRC-generated C. Current
   `make -C tests graph-coverage` is operation coverage only — document any real
   coverage setup separately.

4. Keep docs current. Update `README.md` and this file when changing the graph,
   host naming, app settings, immutability behavior, or the live helper.

## Commit Guidance

Before committing:

```bash
git status --short
make build
make check
nix flake check --all-systems --no-build
```

Commit from the repo root. Do not include ignored `.vm`, `.btrc-cache`,
`.DS_Store`, or local secret/config artifacts, and do not stage `audio.nix`.
