# NixOS Configuration

This repository is the flake-backed NixOS configuration for the machines in
`modules/hosts`, plus a staged BTRC rewrite of the local management tooling.
The long-term target is Nix plus BTRC for system management, with Python/Rust
kept only where the current system still needs a legacy implementation or where
external patch workflows require another language.

## Current Hosts

| Host | System | Purpose |
|---|---|---|
| `FRACTAL-NORTH` | `x86_64-linux` | Main desktop/workstation configuration |
| `QEMU` | `aarch64-linux` | Declarative ARM guest used for Nix evaluation and VM-oriented testing |

Each host is exposed with three flake targets:

| Target suffix | Purpose |
|---|---|
| `Disk-Operation` | Disk layout only, used by `disko` during installation |
| `Standard-Boot` | Full system with systemd-boot |
| `Secure-Boot` | Full system with Lanzaboote secure boot |

Examples:

```bash
nix eval .#nixosConfigurations.FRACTAL-NORTH-Standard-Boot.config.system.build.toplevel.drvPath
nix eval .#nixosConfigurations.QEMU-Standard-Boot.config.networking.hostName
nix eval .#nixosConfigurations.QEMU-Disk-Operation.config.disko.devices.disk.main.device
```

`Disk-Operation` targets are disko entrypoints, not installed boot targets. They
import `modules/system/disk-operation.nix`, which wraps the disk layout with the
minimal boot/user defaults needed for the NixOS module graph to evaluate cleanly.

## Repository Layout

| Path | Purpose |
|---|---|
| `flake.nix` | Host discovery, `nixosConfigurations`, and the pinned BTRC compiler input |
| `modules/settings.nix` | Central typed settings interface shared across hosts and modules |
| `modules/system/` | Shared NixOS system modules |
| `modules/apps/` | Globally imported app and helper modules; each optional app is settings-gated |
| `modules/apps/pkg-overrides/` | Custom package overrides for Proton, Rocksmith assets, Sunshine, and Battle.net helper tooling |
| `modules/hosts/x86_64/FRACTAL-NORTH/` | Main workstation host modules and host-specific data |
| `modules/hosts/aarch64/QEMU/` | ARM QEMU guest host modules |
| `scripts/` | Legacy Python/Rust management implementation retained for parity and v1 immutability support |
| `btrc/` | BTRC rewrite of the management layer and VM e2e framework |
| `shell.nix` | Development shell for the legacy Python test path |
| `FOR_CLAUDE.md` | Handoff notes for future Claude Code sessions |

Host discovery is convention-based:

```text
modules/hosts/<architecture>/<HOST>/<HOST>.nix
```

That convention is why `modules/hosts/aarch64/QEMU/QEMU.nix` is discovered.

## Settings Model

`modules/settings.nix` defines the repo's stable configuration interface. Host
modules should prefer overriding `settings.*` instead of directly changing
shared modules whenever a value is host policy rather than module structure.

Important setting groups:

| Setting prefix | Purpose |
|---|---|
| `settings.user.admin.*` | Single-admin username, public identity, authorized keys, autologin, lock behavior, Home Manager enablement |
| `settings.disk.*` | Disk device, partition labels, btrfs subvolumes, swap, encryption, immutability |
| `settings.boot.*` | Boot method, secure-boot PKI bundle, generation limit, timeout |
| `settings.tpm.*` | TPM2 device and version paths |
| `settings.desktop.*` | Display outputs, cursor theme/package/path, Plasma theme values |
| `settings.apps.*` | Optional app policy flags |
| `settings.networking.*` | LAN subnet, firewall port intent, SSH identity agent, primary NIC |
| `settings.input.*` | Libinput mouse overrides |
| `settings.rocksmith.*` | Audio buffer/sample rate used by Steam/Rocksmith and low-latency PipeWire config |
| `settings.nixosHelper.*` | NixOS helper config path |
| `settings.sudolessAllowlist.*` | Optional sudo NOPASSWD command/package allowlist |

### App Flags

Every global app module has a default-on settings switch:

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

Use these for per-host or per-profile policy. Architecture checks are still
capability guards. Battle.net and Rocksmith require their own flag,
`settings.apps.steam.enable`, and an x86_64 host platform. Sunshine's custom
package overlay is x86_64-gated; non-x86 systems keep upstream `pkgs.sunshine`
when referenced.

Plasma desktop state is intentionally not treated as an optional app flag. It is
part of the shared desktop surface, not an app install.

## System Modules

### Boot

`modules/system/boot.nix` selects behavior from `settings.boot.method`.

| Method | Behavior |
|---|---|
| `Standard-Boot` | Enables systemd-boot |
| `Secure-Boot` | Disables systemd-boot directly and enables Lanzaboote |
| `Disk-Operation` | Not handled by this module; the flake imports `modules/system/disk-operation.nix` for this target |

The shared boot module sets current kernel packages, Plymouth, quiet boot
parameters, EFI mount point, and generation limits.

### Disk

`modules/system/disk.nix` owns the `disko` layout. The default model is:

| Layer | Default |
|---|---|
| Partition table | GPT |
| EFI partition | `512M`, vfat, mounted at `settings.disk.boot.efiSysMountPoint` |
| Root partition | btrfs, optionally wrapped in LUKS |
| Subvolumes | `@root`, `@home`, `@nix`, `@var`, `@snapshots`, `@swap` |

The QEMU host overrides the disk to `/dev/vda`, disables encryption, enables
semipermeable immutability v2, and sets a small swap size suitable for VM use.

### Immutability

`modules/system/immutability.nix` supports two implementations:

| Implementation | Status |
|---|---|
| `v1` | Original Rust initrd reset implementation, retained for existing systems only |
| `semipermeable_membrane` | BTRC-generated C implementation, used by new BTRC tests and QEMU |

The BTRC semipermeable membrane is generated into
`btrc/generated/semipermeable_membrane.c` and built by Nix as a C program.
The original Rust v2 prototype remains under `scripts/lib/semipermeable_membrane.rs`
as historical/reference code, but the live v2 package points at the BTRC output.

The v2 mode persists selected files/directories by materializing persistent
btrfs subvolumes under `@persist` and bind-mounting selected targets after
reset. The source-of-truth persist list is
`settings.disk.immutability.persist.paths`.

### Desktop

`modules/system/desktop.nix` enables the Plasma 6 Wayland desktop, SDDM, dconf,
graphics, cursor packages, and selected KDE integrations. It also overlays a
KWin HDR screencast patch used by the Sunshine workflow.

Host-specific desktop state lives in the host modules:

| File | Role |
|---|---|
| `modules/hosts/x86_64/FRACTAL-NORTH/gpu.nix` | NVIDIA/AMD graphics setup, device symlinks, KWin DRM devices |
| `modules/hosts/x86_64/FRACTAL-NORTH/audio.nix` | Low-latency PipeWire and device-specific audio rules |
| `modules/hosts/x86_64/FRACTAL-NORTH/input.nix` | Controller and mouse receiver support |
| `modules/hosts/x86_64/FRACTAL-NORTH/sunshine.nix` | Sunshine service, streaming display EDID, streaming display setup |

### Networking And Security

`modules/system/networking.nix` owns NetworkManager, firewall rules, mDNS,
OpenSSH, SSH agent forwarding, and optional primary-interface enforcement.
Firewall LAN allow rules are derived from shared service flags and
`settings.networking.ports.{tcp,udp}`.

`modules/system/admin.nix` imports Home Manager and every top-level app module
under `modules/apps`. It defines the immutable admin user and Home Manager
baseline. User mutability is disabled, so password changes must update the
hashed password file and rebuild.

`modules/system/sudolessAllowlist.nix` is opt-in through
`settings.sudolessAllowlist.enable`.

## Apps And Package Overrides

The app layer is globally imported but policy-gated through `settings.apps`.

| Module | Behavior |
|---|---|
| `1password.nix` | Installs 1Password GUI/CLI and writes SSH agent config |
| `bash.nix` | Adds `nix-shell-with-pkgs` helper |
| `battlenet.nix` | Installs a Battle.net Proton wrapper, desktop entry, and capture helper on x86_64 when Steam is enabled |
| `claude.nix` | Installs a Claude skill that teaches Claude how to load this repo |
| `cursor.nix` | Symlinks the configured cursor theme into user icon paths |
| `git.nix` | Configures Git identity |
| `helper.nix` | Installs the legacy Python `nixos` and `nixos-helper` wrappers until BTRC replaces the live desktop helper |
| `rclone.nix` | Defines a user rclone mount for `~/Drive` |
| `steam.nix` | Exports Steam tool names and configures installed Steam apps when Steam is enabled |
| `sunshine.nix` | Overlays the custom Sunshine package on x86_64 and sets runtime library paths when Sunshine is enabled |
| `vscode.nix` | Installs VSCode from the unstable package set |

The custom Proton tree under `modules/apps/pkg-overrides/proton-custom` is a
large patch-carrying workflow for Battle.net/Wayland/HDR behavior. Keep that
area focused and avoid broad formatting churn; its local README has the
development loop.

VFIO has been removed from this repo for now because the path was broken and
explicitly out of scope for the BTRC rewrite.

## BTRC Management Layer

`btrc/` is the staged replacement for the Python/Rust management layer.
The compiler is consumed through the flake input `inputs.btrc`, currently pinned
to the sibling BTRC git checkout with a local `git+file://` URL. The local Makefile builds through
`nix run ..#btrc` and `nix develop ..#btrc-build`, so the dependency is explicit
in the flake graph rather than an implicit submodule or ad hoc `cd ../../btrc`.

| Path | Purpose |
|---|---|
| `btrc/bin/nixosctl.btrc` | Main CLI |
| `btrc/bin/semipermeable_membrane.btrc` | BTRC entrypoint for the v2 immutability runtime |
| `btrc/src/core` | Logging, paths, interactive prompts |
| `btrc/src/nix` | Config eval, rebuild/update, secrets, permissions, password changes |
| `btrc/src/btrfs` | Snapshot and diff helpers |
| `btrc/src/install` | Install orchestration |
| `btrc/src/hardware` | Secure Boot and TPM2 helpers |
| `btrc/src/desktop` | Audio, display, caffeine, labels, terminal launch helpers |
| `btrc/src/immutability` | Semipermeable membrane v2 |
| `btrc/vendor/btrc-stdlib` | Vendored BTRC stdlib used for this rewrite |
| `btrc/tests/e2e` | Declarative VM graph and test runner |

Build from the repo root:

```bash
make -C btrc build
make -C btrc generated
make -C btrc stdlib-sync-check
```

Common BTRC commands after build:

```bash
btrc/build/nixosctl eval config.networking.hostName
sudo btrc/build/nixosctl update --upgrade
sudo btrc/build/nixosctl install --format
sudo btrc/build/nixosctl diff --pattern '/var/*'
sudo btrc/build/nixosctl fix-permissions
sudo btrc/build/nixosctl secure-boot status
sudo btrc/build/nixosctl tpm2 status
```

The legacy Python wrappers are still installed by `modules/apps/helper.nix`.
They remain the live desktop helper until the BTRC GUI/tray daemon is complete.

## E2E VM Testing

The BTRC test framework is declarative. Specs live under `btrc/tests/*` and the
graph lives at `btrc/tests/graph.json`.

Core ideas:

| Concept | Meaning |
|---|---|
| Spec | JSON, TOML, or strict YAML file declaring VM/host operations |
| Graph node | Named spec plus dependencies and optional args |
| State hash | Hash of parent hash, state name, material, and operations |
| State root | `.vm/e2e/chain/<state>` with recorded hashes and workdir metadata |
| qcow2 overlay | Child VM disk can be reconstructed from parent state without rerunning installation |

Useful test targets:

```bash
make -C btrc quick
make -C btrc app-settings
make -C btrc aarch64-qemu-host
make -C btrc graph-coverage
make -C btrc graph-early
make -C btrc installer-download
make -C btrc installer-ssh
make -C btrc graph-installer-ssh
make -C btrc graph-full
```

Targets that boot QEMU or install NixOS can take time. The early graph is the
right default for normal development. The full graph should be used before
trusting changes to install, immutability, TPM2, or Secure Boot behavior.

The operation catalog currently has full declared operation coverage through
`make -C btrc graph-coverage`. That is operation coverage, not generated-C line
coverage.

## Development Workflow

Recommended loop:

```bash
git status --short
nix eval .#nixosConfigurations.FRACTAL-NORTH-Standard-Boot.config.system.build.toplevel.drvPath --show-trace
nix eval .#nixosConfigurations.QEMU-Standard-Boot.config.system.build.toplevel.drvPath --show-trace
make -C btrc build
make -C btrc graph-coverage
make -C btrc app-settings
make -C btrc aarch64-qemu-host
```

For all flake outputs:

```bash
nix flake check --all-systems --no-build --show-trace
```

For disk-only targets, evaluate disk attributes directly:

```bash
nix eval .#nixosConfigurations.QEMU-Disk-Operation.config.disko.devices.disk.main.device --show-trace
nix eval .#nixosConfigurations.QEMU-Disk-Operation.config.disko.devices.disk.main.content.partitions.root.content.type --show-trace
```

For Nix linting:

```bash
nix run nixpkgs#deadnix -- .
nix run nixpkgs#statix -- check .
```

`statix` still reports some style-only warnings in dense settings and legacy
modules. Prefer fixing warnings when touching nearby code, but do not churn
large settings blocks solely for style.

## Secrets And Local State

Ignored local paths:

| Path | Purpose |
|---|---|
| `secrets/` | Local secret material such as hashed password files |
| `config.json` | Local selected host/target for helper tooling |
| `.vm/` and `btrc/.vm/` | QEMU disks, ISO downloads, state graph data |
| `.direnv/`, `.pytest_cache/`, `.ruff_cache/`, `__pycache__` | Development caches |
| `result` | Nix build symlink |

Do not commit secret material, VM disk images, installer ISOs, or generated
Python caches.

## Migration Notes

The BTRC parity ledger is `btrc/PARITY.md`. As of this pass:

| Area | State |
|---|---|
| CLI dispatch | Ported to BTRC |
| Nix eval/update/install/diff/permissions | Ported to BTRC |
| Audio/display/caffeine/system helper backends | Ported to BTRC |
| TPM2 and Secure Boot helpers | Ported to BTRC with VM capability coverage |
| Semipermeable immutability v2 | Implemented in BTRC and checked in as generated C for Nix builds |
| GUI tray/window daemon | Still needs a persistent native/WebView backend |
| VFIO | Removed for now |
| Python/Rust legacy tree | Retained for parity/reference and v1 immutability only |

## Maintenance Rules

1. Prefer `settings.*` for host policy.
2. Keep architecture-specific behavior as capability gates, not broad host-level removal.
3. Keep app installs under `settings.apps.*`.
4. Keep QEMU-specific aarch64 assumptions inside `modules/hosts/aarch64/QEMU`.
5. Do not add tests that only assert that broken or unimplemented behavior is absent.
6. Add positive tests for expected behavior.
7. Run at least the BTRC build and relevant graph node after touching BTRC or VM test code.
8. Do not reintroduce VFIO until there is a clean design and expected-behavior test path.
9. Do not remove desktop packages or host customizations while making cross-platform changes unless they are concretely unsupported on that platform.
