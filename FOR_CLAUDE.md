# FOR CLAUDE CODE

This file is a handoff for future Claude Code sessions working on this NixOS
repository.

## Current Intent

The user wants to migrate local management tooling from Python/Rust to BTRC
while keeping the NixOS configuration clean, reproducible, and cross-platform
where practical. The end state is Nix plus BTRC for system management.

The user has been explicit about these preferences:

- Do not ask for permission unless there is a real blocker.
- Do not stop at a plan when implementation is possible.
- Use BTRC idioms in BTRC code.
- Prefer adding stdlib wrappers instead of dropping into C-style code at call sites.
- Use declarative, graph-based VM e2e tests.
- Test expected behavior, not negative assertions for things that were never implemented correctly.
- Do not remove NixOS desktop/application configuration casually.
- Gate x86-specific things narrowly and preserve host customizations.
- VFIO is out of scope for now and should not be reintroduced without a fresh design.

## Repository Overview

| Path | Meaning |
|---|---|
| `flake.nix` | Builds every host and boot target; pins the sibling BTRC compiler input |
| `modules/settings.nix` | Stable settings API |
| `modules/system/` | Shared system modules |
| `modules/apps/` | Settings-gated app modules |
| `modules/hosts/x86_64/FRACTAL-NORTH/` | Main workstation |
| `modules/hosts/aarch64/QEMU/` | ARM QEMU test host |
| `btrc/` | BTRC rewrite and VM harness |
| `scripts/` | Legacy Python/Rust implementation retained for parity/reference |
| `README.md` | Main detailed docs |
| `btrc/README.md` | BTRC and VM harness docs |
| `btrc/PARITY.md` | Side-by-side migration ledger |

Host discovery convention:

```text
modules/hosts/<arch>/<HOST>/<HOST>.nix
```

The aarch64 host is intentionally uppercase `QEMU`, matching the
`FRACTAL-NORTH` naming style.

The BTRC compiler is an explicit flake input:

```nix
inputs.btrc.url = "github:schiffy91/btrc";
```

It is pinned in `flake.lock` to the published BTRC commit on GitHub. Local
development can still iterate against the sibling `~/Drive/dev/btrc` checkout;
re-point the input and run `nix flake update btrc` after pushing to re-pin. This
does not change the `btrc/Makefile` contract.

## Important Current State

The current working set includes:

- BTRC management tree under `btrc/`.
- BTRC-generated semipermeable membrane C at `btrc/generated/semipermeable_membrane.c`.
- `btrc/Makefile` builds via `nix run ..#btrc` and `nix develop ..#btrc-build`.
- Live Nix v2 immutability build now points at the generated BTRC C file.
- Original Rust v1 immutability remains under `scripts/lib/immutability.rs`.
- Original Rust v2 prototype remains under `scripts/lib/semipermeable_membrane.rs` as reference only.
- `modules/hosts/x86_64/FRACTAL-NORTH/vfio.md` is deleted.
- Stale loose `modules/hosts/aarch64/Venus.nix` was removed because it did not match host discovery and referenced nonexistent desktop settings.
- Unused Rocksmith debug helper `modules/apps/pkg-overrides/rocksmith/monitor_audio_inputs.py` was removed because nothing referenced or installed it.

## App Settings

Optional app policy is controlled through `settings.apps.*` in
`modules/settings.nix`:

```nix
settings.apps.enable = true;
settings.apps.onePassword.enable = true;
settings.apps.bash.enable = true;
settings.apps.battlenet.enable = true;
settings.apps.claude.enable = true;
settings.apps.cursor.enable = true;
settings.apps.git.enable = true;
settings.apps.nixosHelper.enable = true;
settings.apps.rclone.enable = true;
settings.apps.rocksmith.enable = true;
settings.apps.steam.enable = true;
settings.apps.sunshine.enable = true;
settings.apps.vscode.enable = true;
```

Battle.net and Rocksmith are gated by their own flags, `settings.apps.steam`,
and `pkgs.stdenv.hostPlatform.isx86_64`. Sunshine's custom package overlay is
also x86_64-gated; non-x86 platforms keep upstream `pkgs.sunshine` if referenced.

Do not solve app portability by disabling the whole desktop stack. Gate only the
specific unsupported app or package override.

## BTRC Style Notes

BTRC code should look like BTRC, not C pasted into BTRC:

- Prefer classes and small managers.
- Prefer `Command(...).arg(...).capture(...).check(...)` chaining.
- Use `UnixShell` for shell execution.
- Use `FileSystem`, `Path`, `PathTools`, `JsonObject`, `Toml`, `Vector`, `Map`,
  and other stdlib wrappers before writing raw shell or raw C-like logic.
- Keep platform-specific shells explicit. The implemented backend is
  `UnixShell`; `PowerShell` remains TODO.
- If there is no wrapper and the operation belongs in stdlib, add a wrapper.
- Avoid `strcmp`-style call sites in app code.
- Prefer f-strings over `"a" + b + "c"` when the expression is clearer.

The BTRC compiler supports imports, but `btrc/bin/nixosctl.btrc` still uses an
explicit include order because this vendored build is compiled with
`--no-stdlib`. Do not assume automatic global scope is a stable design.

## Nix Style Notes

Use the existing module style, with these preferences:

- Put policy in `settings.*`.
- Use `lib.mkIf` for optional module behavior.
- Use `lib.mkDefault` when a host should be able to override a default.
- Use `lib.meta.availableOn` for package availability when that is the real question.
- Keep host-specific hardware details in the host folder.
- Keep QEMU-specific boot/firmware details in `modules/hosts/aarch64/QEMU`.
- Avoid broad `disabledModules` or "turn off desktop" shortcuts.
- Do not remove packages such as Breeze, Plasma bits, 1Password, VSCode, rclone, or helper tooling unless there is a direct requested reason.

## E2E VM Framework

The BTRC VM framework lives under `btrc/tests/e2e`.

Key commands:

```bash
make -C btrc build
make -C btrc graph-coverage
make -C btrc app-settings
make -C btrc aarch64-qemu-host
make -C btrc graph-early
make -C btrc graph-installer-ssh
make -C btrc graph-full
```

Graph file:

```text
btrc/tests/graph.json
```

Important nodes:

| Node | Purpose |
|---|---|
| `quick` | cheap harness sanity |
| `cli-parity` | host-side CLI parity |
| `spec-toml` / `spec-yaml` | spec parser coverage |
| `app-settings` | app settings and host service expectations |
| `aarch64-qemu-host` | evaluates the uppercase `QEMU` host |
| `installer-download` | downloads the NixOS minimal ISO |
| `installer-ssh` | boots installer ISO and injects SSH |
| `installer-setup` | validates setup op |
| `installer-up-iso` | validates up-iso op |
| `qmp-probe` | validates QMP |
| `tpm2-probe` | validates swtpm plus guest TPM2 |
| `install-reset` | installs reset-mode v2 immutability |
| `install-disabled` | installs with immutability disabled |
| `immutability-reset` | boots installed disk and validates reset (ephemeral wiped, persist kept) |
| `immutability-disabled` | boots disabled install and validates nothing is wiped |
| `immutability-snapshot-only` | switches to snapshot-only mode; validates ephemeral survives rotation |
| `immutability-restore` | reset boots then restore-previous (slot A) / restore-penultimate (slot B) |
| `secure-boot-capabilities` | x86_64 secure boot capability probe |
| `secure-boot-install` | optional x86_64 Lanzaboote install |
| `secure-boot-lanzaboote` | optional x86_64 Lanzaboote verification |

`make -C btrc graph-coverage` currently reports `35/35` operation coverage.
That means every declared operation kind is represented in graph specs. It is
not generated-C line coverage.

### Membrane behavior notes (validated via the e2e suite)

- `restore-previous` restores rotation slot A, `restore-penultimate` slot B;
  `restore-a`/`restore-b`/`restore-c` are the explicit internal modes.
- A reset persists each `persist.paths` directory into `@persist/dirs/<key>`
  and the `semipermeable-membrane-mounts` service mounts it back over the path.
  Pre-existing content is preserved across the first reset.
- The headless test VM is selected by `settings.desktop.enable = false`,
  wired from the harness `desktop=none` arg; real hosts default to `true`.
- BTRC gotcha worth remembering: `for x in self.method()` re-evaluates the call
  on every iteration. Snapshot a list-returning call into a local before the
  loop, especially when the body mutates what that call observes (deleting
  btrfs subvolumes inside such a loop previously segfaulted on the shrinking
  vector and leaked the old root subvolume each boot).

## Validation Commands

Use these before committing broad changes:

```bash
make -C btrc build
make -C btrc graph-coverage
make -C btrc app-settings
make -C btrc aarch64-qemu-host
nix flake check --all-systems --no-build --show-trace
```

Use direct disk evals for disk-only targets:

```bash
nix eval .#nixosConfigurations.QEMU-Disk-Operation.config.disko.devices.disk.main.device --show-trace
nix eval .#nixosConfigurations.QEMU-Disk-Operation.config.disko.devices.disk.main.content.partitions.root.content.type --show-trace
```

Optional lint passes:

```bash
nix run nixpkgs#deadnix -- .
nix run nixpkgs#statix -- check .
```

`statix` still has low-value style warnings in dense settings expressions.
Fix warnings near code you are already touching; do not churn the whole settings
file just to satisfy style output.

## Known Caveats

- `Disk-Operation` targets are disko entrypoints, not installed boot targets.
  They use `modules/system/disk-operation.nix` so `nix flake check` can still
  evaluate every `nixosConfiguration`.
- Secure Boot enforcement in QEMU is currently x86_64-specific because secure
  EDK2 firmware availability is the limiting capability.
- The live desktop helper still uses Python wrappers from `modules/apps/helper.nix`.
  BTRC backend functionality exists, but the GUI/tray event loop is not finished.
- Python/Rust under `scripts/` is not all dead code yet. Some pieces are parity
  reference, some are the legacy live helper, and Rust v1 immutability remains
  intentionally available.
- `btrc/generated/semipermeable_membrane.c` is checked in because the Nix module
  needs generated C during system evaluation; the compiler itself is now
  packaged through the BTRC flake input (`github:schiffy91/btrc`).
- The user may have an unrelated host QEMU process from another app. Do not
  kill host QEMU processes unless you can identify them as this repo's harness.

## Immediate TODOs

1. Finish the BTRC GUI/tray daemon backend.
   - Keep the API declarative.
   - Use stdlib `Ui*`, `NativeUiBackend`, `Daemon*`, and `Command`.
   - Prefer native WebView or native widgets behind a stable interface.

2. Add true coverage instrumentation for BTRC-generated C.
   - Current `graph-coverage` is operation coverage only.
   - A real line/path coverage setup should be separate and documented clearly.

3. Continue shrinking Python live usage.
   - Replace `modules/apps/helper.nix` Python wrappers with the BTRC `nixosctl`
     binary after the GUI/tray story is ready.
   - Keep old Python around only as reference until parity is proven.

4. (Done) The NixOS input sources BTRC from `github:schiffy91/btrc`, pinned in
   `flake.lock` at the pushed commit. `make -C btrc build` still flows through
   `nix run ..#btrc`, and `make -C btrc stdlib-sync-check` confirms the vendored
   stdlib matches `..#btrcSrc`.
   - Relative `git+file:../btrc` was tested and rejected because current Nix
     locks it but can mis-fetch it during eval; the GitHub URL avoids that.

5. Harden Secure Boot VM coverage.
   - Keep x86_64 capability checks.
   - Avoid pretending aarch64 Secure Boot works until the firmware path is real.

6. Keep docs current.
   - Update `README.md`, `btrc/README.md`, and this file when changing the
     graph, host naming, app settings, immutability, or live helper entrypoints.

## Commit Guidance

Before committing:

```bash
git status --short
make -C btrc build
make -C btrc graph-coverage
make -C btrc app-settings
make -C btrc aarch64-qemu-host
```

Then commit from the repo root. A reasonable commit message for the current
migration batch is:

```text
Migrate NixOS management scaffolding to BTRC
```

Do not include ignored `.vm`, `.pytest_cache`, `.DS_Store`, `__pycache__`, or
local secret/config artifacts.
