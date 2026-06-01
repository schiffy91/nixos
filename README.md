# NixOS Configuration

This repository is the flake-backed NixOS configuration for the machines in
`nix/hosts`, with local management tooling written in BTRC. System
management is Nix plus BTRC end to end; there is no Python or Rust in the
management layer (the only Python left is an x86-gated Sunshine EDID packaging
helper under `nix/apps/pkg-overrides/`).

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
import `nix/system/disk-operation.nix`, which wraps the disk layout with the
minimal boot/user defaults needed for the NixOS module graph to evaluate cleanly.

## Repository Layout

| Path | Purpose |
|---|---|
| `flake.nix` | Host discovery, `nixosConfigurations`, and the pinned BTRC compiler input |
| `nix/settings.nix` | Central typed settings interface shared across hosts and modules |
| `nix/system/` | Shared NixOS system modules |
| `nix/apps/` | Globally imported app and helper modules; each optional app is settings-gated |
| `nix/apps/pkg-overrides/` | Custom package overrides for Proton, Rocksmith assets, Sunshine, and Battle.net helper tooling |
| `nix/hosts/x86_64/FRACTAL-NORTH/` | Main workstation host modules and host-specific data |
| `nix/hosts/aarch64/QEMU/` | ARM QEMU guest host modules |
| `btrc/system/` | BTRC management CLI entrypoint and system helpers |
| `btrc/immutability/` | BTRC immutability entrypoint and engine modules |
| `btrc/btrfs/`, `btrc/core/`, `btrc/desktop/`, `btrc/hardware/`, `btrc/install/` | BTRC support libraries grouped by subsystem |
| `build/` | Gitignored local compiler intermediates and binaries |
| `inputs.btrc` | Flake-provided BTRC compiler and precompiled stdlib archive source |
| `tests/` | Declarative VM e2e graph and per-scenario specs |
| `Makefile` | Root build/test contract (`make build`, `make check`, `make -C tests graph-*`) |
| `FOR_CLAUDE.md` | Handoff notes for future Claude Code sessions |

Host discovery is convention-based:

```text
nix/hosts/<architecture>/<HOST>/<HOST>.nix
```

That convention is why `nix/hosts/aarch64/QEMU/QEMU.nix` is discovered.

## Settings Model

`nix/settings.nix` defines the repo's stable configuration interface. Host
modules should prefer overriding `settings.*` instead of directly changing
shared modules whenever a value is host policy rather than module structure.

Important setting groups:

| Setting prefix | Purpose |
|---|---|
| `settings.users.admin.*` | Admin username, public identity, authorized keys, autologin, lock behavior, Home Manager enablement |
| `settings.users.agent.*` | Isolated agent user, packages, SSH keys, and persisted workspace/cache paths |
| `settings.disk.*` | Disk device, partition labels, btrfs subvolumes, swap, encryption, immutability |
| `settings.boot.*` | Boot method, secure-boot PKI bundle, generation limit, timeout |
| `settings.tpm.*` | TPM2 device and version paths |
| `settings.desktop.*` | Display outputs, cursor theme/package/path, Plasma theme values |
| `settings.apps.*` | Optional app policy flags |
| `settings.networking.*` | LAN subnet, firewall port intent, SSH identity agent, primary NIC |
| `settings.input.*` | Libinput mouse overrides |
| `settings.rocksmith.*` | Audio buffer/sample rate used by Steam/Rocksmith and low-latency PipeWire config |
| `settings.nixosctl.*` | `nixosctl` config path |
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
settings.apps.nixosctl.enable = true;
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

`nix/system/boot.nix` selects behavior from `settings.boot.method`.

| Method | Behavior |
|---|---|
| `Standard-Boot` | Enables systemd-boot |
| `Secure-Boot` | Disables systemd-boot directly and enables Lanzaboote |
| `Disk-Operation` | Not handled by this module; the flake imports `nix/system/disk-operation.nix` for this target |

The shared boot module sets current kernel packages, Plymouth, quiet boot
parameters, EFI mount point, and generation limits.

### Disk

`nix/system/disk.nix` owns the `disko` layout. The default model is:

| Layer | Default |
|---|---|
| Partition table | GPT |
| EFI partition | `512M`, vfat, mounted at `settings.disk.boot.efiSysMountPoint` |
| Root partition | btrfs, optionally wrapped in LUKS |
| Subvolumes | `@root`, `@home`, `@nix`, `@var`, `@snapshots`, `@swap` |

The QEMU host overrides the disk to `/dev/vda`, disables encryption, enables
the immutability reset workflow, and sets a small swap size suitable for VM use.

### Immutability

`nix/system/immutability.nix` drives the BTRC **immutability**, the
sole immutability implementation. Its source entrypoint is
`btrc/immutability/immutability.btrc`; Nix builds it with the flake-pinned
BTRC compiler and a generated stdlib archive.

| Mode | Behavior |
|---|---|
| `reset` | Roll each `resetOnBoot` subvolume back to its read-only `CLEAN` snapshot at boot |
| `snapshot-only` | Rotate snapshots without rolling back (ephemeral survives) |
| `restore-previous` / `restore-penultimate` | Roll back to rotation slot A / B |
| `disabled` | No reset |

`enforce.onReboot` wires the initrd reset + mounts service; `enforce.onUpdate`
adds a `snapshot-clean` activation that re-captures the `CLEAN` baseline at
`nixos-rebuild switch`. Orphaned `@persist/dirs/<key>` subvolumes (keys no longer
in the spec) are pruned on a reconciling reset.

The v2 mode persists selected files/directories by materializing persistent
btrfs subvolumes under `@persist` and bind-mounting selected targets after
reset. The source-of-truth persist list is
`settings.disk.immutability.persist.paths`.

### Desktop

`nix/system/desktop.nix` enables the Plasma 6 Wayland desktop, SDDM, dconf,
graphics, cursor packages, and selected KDE integrations. It also overlays a
KWin HDR screencast patch used by the Sunshine workflow.

Host-specific desktop state lives in the host modules:

| File | Role |
|---|---|
| `nix/hosts/x86_64/FRACTAL-NORTH/gpu.nix` | NVIDIA/AMD graphics setup, device symlinks, KWin DRM devices |
| `nix/hosts/x86_64/FRACTAL-NORTH/audio.nix` | Low-latency PipeWire and device-specific audio rules |
| `nix/hosts/x86_64/FRACTAL-NORTH/input.nix` | Controller and mouse receiver support |
| `nix/hosts/x86_64/FRACTAL-NORTH/sunshine.nix` | Sunshine service, streaming display EDID, streaming display setup |

### Networking And Security

`nix/system/networking.nix` owns NetworkManager, firewall rules, mDNS,
OpenSSH, SSH agent forwarding, and optional primary-interface enforcement.
Firewall LAN allow rules are derived from shared service flags and
`settings.networking.ports.{tcp,udp}`.

`nix/system/users.nix` imports Home Manager and every top-level app module
under `nix/apps`. It defines immutable users: the interactive admin account and
an isolated agent account with its own Home Manager profile and tool set. User
mutability is disabled, so password changes must update the hashed password file
and rebuild.

`nix/system/sudolessAllowlist.nix` is opt-in through
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
| `git.nix` | Enables system Git; per-user identities live in Home Manager |
| `nixosctl.nix` | Builds + packages the BTRC `nixosctl` CLI into `environment.systemPackages` |
| `rclone.nix` | Defines a user rclone mount for `~/Drive` |
| `steam.nix` | Exports Steam tool names and configures installed Steam apps when Steam is enabled |
| `sunshine.nix` | Overlays the custom Sunshine package on x86_64 and sets runtime library paths when Sunshine is enabled |
| `vscode.nix` | Installs VSCode from the unstable package set |

The custom Proton tree under `nix/apps/pkg-overrides/proton-custom` is a
large patch-carrying workflow for Battle.net/Wayland/HDR behavior. Keep that
area focused and avoid broad formatting churn; its local README has the
development loop.

VFIO has been removed from this repo for now because the path was broken and
explicitly out of scope for the BTRC rewrite.

## BTRC Management Layer

The management tooling is BTRC at the repository root. The compiler is consumed
through the flake input `inputs.btrc`, pinned in `flake.lock` to
`github:schiffy91/btrc`. The dev shell provides `btrcpy`; local and Nix builds
compile with `--strict-imports` against a generated stdlib archive.

| Path | Purpose |
|---|---|
| `btrc/system/nixosctl.btrc` | Main CLI (also hosts the VM e2e harness: `nixosctl graph` / `e2e`) |
| `btrc/immutability/immutability.btrc` | BTRC entrypoint for the immutability runtime |
| `btrc/btrfs` | Snapshot and diff helpers |
| `btrc/immutability` | Immutability |
| `tests/e2e` | Declarative VM graph and test runner |

Build from the repo root:

```bash
make build              # transpile + compile nixosctl and immutability
make clean              # remove gitignored compiler outputs
```

Common `nixosctl` commands after build:

```bash
build/nixosctl eval config.networking.hostName
sudo build/nixosctl update --upgrade
sudo build/nixosctl install --format
sudo build/nixosctl diff --pattern '/var/*'
sudo build/nixosctl fix-permissions
sudo build/nixosctl secure-boot status
sudo build/nixosctl tpm2 status
```

On installed hosts the CLI is packaged as `nixosctl` (see `nix/apps/nixosctl.nix`)
and is on `PATH`. The native system tray (macOS menu bar / Linux Wayland
StatusNotifierItem) is implemented in the BTRC stdlib and packaged by
`nix/apps/nixos-tray.nix`.

## E2E VM Testing

The BTRC test framework is declarative. Specs live under `tests/*` and the graph
lives at `tests/graph.json`. The runner is the `nixosctl` binary itself
(`nixosctl graph tests/graph.json run <node>`).

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
make -C tests quick
make -C tests app-settings
make -C tests aarch64-qemu-host
make -C tests graph-coverage
make -C tests installer-download
make -C tests installer-ssh
make -C tests graph-full
```

Immutability/CLI scenario nodes: `immutability-reset`, `immutability-disabled`,
`immutability-snapshot-only`, `immutability-restore`, `immutability-orphan`,
`immutability-files`, `immutability-onupdate`, `nixosctl-cli`, `nixosctl-diff`.
Hardware nodes: `tpm2-probe`, `tpm2-enroll` (LUKS TPM2 enroll/wipe on an
encrypted install), `secure-boot-aarch64` (full Lanzaboote Secure Boot on
aarch64 under HVF — install, sign, auto-enroll keys, then `bootctl status` →
`Secure Boot: enabled`; uses Debian's enforcing `AAVMF_CODE.secboot.fd` via a
reproducible nix derivation in `tests/shell.nix`), and the x86_64
`secure-boot-*` nodes.

Targets that boot QEMU or install NixOS can take time. The early graph is the
right default for normal development. The full graph should be used before
trusting changes to install, immutability, TPM2, or Secure Boot behavior.

The operation catalog currently has full declared operation coverage through
`make -C tests graph-coverage`. That is operation coverage, not generated-C line
coverage.

## Development Workflow

Recommended loop:

```bash
git status --short
nix eval .#nixosConfigurations.FRACTAL-NORTH-Standard-Boot.config.system.build.toplevel.drvPath --show-trace
nix eval .#nixosConfigurations.QEMU-Standard-Boot.config.system.build.toplevel.drvPath --show-trace
make build
make -C tests graph-coverage
make -C tests app-settings
make -C tests aarch64-qemu-host
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
| `.vm/` | QEMU disks, ISO downloads, state graph data |
| `.direnv/`, `.btrc-cache/`, `build/` | Development caches and build outputs |
| `result` | Nix build symlink |

Do not commit secret material, VM disk images, installer ISOs, or local build
caches.

## Migration Notes

The migration to BTRC is complete. Current state:

| Area | State |
|---|---|
| CLI dispatch | BTRC (`nixosctl`) |
| Nix eval/update/install/diff/permissions | BTRC |
| Audio/display/caffeine/system helper backends | BTRC |
| TPM2 and Secure Boot helpers | BTRC with VM capability coverage |
| Immutability | BTRC source compiled by the flake-pinned compiler |
| Native system tray | Implemented in BTRC stdlib (`../btrc`); helper wiring pending publish |
| VFIO | Removed for now |
| Python / Rust | Removed (only an x86-gated Sunshine EDID packaging helper remains) |

## Maintenance Rules

1. Prefer `settings.*` for host policy.
2. Keep architecture-specific behavior as capability gates, not broad host-level removal.
3. Keep app installs under `settings.apps.*`.
4. Keep QEMU-specific aarch64 assumptions inside `nix/hosts/aarch64/QEMU`.
5. Do not add tests that only assert that broken or unimplemented behavior is absent.
6. Add positive tests for expected behavior.
7. Run at least the BTRC build and relevant graph node after touching BTRC or VM test code.
8. Do not reintroduce VFIO until there is a clean design and expected-behavior test path.
9. Do not remove desktop packages or host customizations while making cross-platform changes unless they are concretely unsupported on that platform.
