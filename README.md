# NixOS Configuration

This repository is the flake-backed NixOS configuration for the machines in
`nix/hosts`, with local management tooling written in BTRC. System
management is Nix plus BTRC end to end; there is no Python or Rust in the
management layer (the only Python left is an x86-gated Sunshine EDID packaging
helper under `nix/apps/gaming/`).

## Current Hosts

| Host | System | Purpose |
|---|---|---|
| `FRACTAL-NORTH` | `x86_64-linux` | Main desktop/workstation configuration |
| `QEMU` | `aarch64-linux` | Declarative ARM guest used for Nix evaluation and VM-oriented testing |
| `QEMU` | `x86_64-linux` | Declarative x86 guest used for Nix evaluation and VM-oriented testing |

Each host is exposed with bootable NixOS targets and disk-operation disko
targets:

| Target suffix | Purpose |
|---|---|
| `Standard-Boot` | Full system with systemd-boot |
| `Secure-Boot` | Full system with Lanzaboote secure boot |
| `Disk-Operation` | Disk layout only, exposed under `diskoConfigurations` for `disko` |

Examples:

```bash
nix eval .#nixosConfigurations.FRACTAL-NORTH-Standard-Boot.config.system.build.toplevel.drvPath
nix eval .#nixosConfigurations.QEMU-aarch64-Standard-Boot.config.networking.hostName
nix eval .#nixosConfigurations.QEMU-x86_64-Standard-Boot.config.networking.hostName
nix eval .#diskoConfigurations.QEMU-x86_64-Disk-Operation.disko.devices.disk.main.device
```

`Disk-Operation` targets are disko entrypoints, not installed boot targets. They
import `nix/system/disk.nix` directly from `flake.nix` and do not appear under
`nixosConfigurations`, so they avoid bootloader and user assertions that only
matter for real NixOS profiles.

## Repository Layout

| Path | Purpose |
|---|---|
| `flake.nix` | Host discovery, bootable targets, disko targets, and the pinned BTRC compiler input |
| `shell.nix` | Direnv-friendly dev shell with `btrcpy`, `btrc-lsp`, Nix tooling, Git, and a C toolchain |
| `nix/settings.nix` | Central typed settings interface shared across hosts and modules |
| `nix/system/` | Shared NixOS system modules |
| `nix/apps/` | Globally imported app and helper modules; each optional app is settings-gated |
| `nix/apps/gaming/` | Custom package overrides for Proton, Rocksmith assets, Sunshine, and Battle.net helper tooling |
| `nix/hosts/x86_64/FRACTAL-NORTH/` | Main workstation host modules and host-specific data |
| `nix/hosts/aarch64/QEMU/` | ARM QEMU guest host modules |
| `nix/hosts/x86_64/QEMU/` | x86 QEMU guest host modules |
| `btrc/nixosctl/` | BTRC management CLI, tray entrypoint, and `nixosctl` feature modules |
| `btrc/immutability/` | BTRC immutability entrypoint and engine modules |
| `btrc/core/` | Shared BTRC helpers used by `nixosctl`, tests, and immutability |
| `build/` | Gitignored local compiler intermediates and binaries |
| `tests/` | Declarative VM e2e graph, per-scenario specs, and flake checks |
| `Makefile` | Root build/test contract (`make build`, `make check`, `make -C tests graph-*`) |

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
settings.apps.dev.enable = true;
settings.apps.gaming.enable = true;
settings.apps.utils.enable = true;
settings.apps.agents.enable = true;
settings.apps.onePassword.enable = true;
settings.apps.bash.enable = true;
settings.apps.battlenet.enable = true;
settings.apps.claude.enable = true;
settings.apps.codex.enable = true;
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
| `Disk-Operation` | Not handled by this module; the flake exposes `nix/system/disk.nix` through `diskoConfigurations` |

The shared boot module sets current kernel packages, Plymouth, quiet boot
parameters, EFI mount point, and generation limits.

### Recovery

`settings.disk.recovery.enable = true` adds a `RECOVERY` vfat partition to the
destructive disko layout and enables the `nixos-recovery-boot-entry` service on
installed systems. The recovery image is not a downloaded ISO; it is a
self-contained NixOS netboot initrd and signed EFI-stub kernel built from
`nix/system/recovery.nix`.

On switch, the service requires `/boot` and `/recovery` to be mounted, then
installs:

| Path | Purpose |
|---|---|
| `/boot/EFI/recovery/kernel.efi` | signed recovery kernel loaded by the installed boot menu |
| `/boot/EFI/recovery/initrd` | self-contained recovery netboot initrd |
| `/boot/loader/entries/nixos-recovery.conf` | systemd-boot/Lanzaboote menu entry |
| `/recovery/EFI/recovery/kernel.efi` | signed recovery kernel copy on the recovery partition |
| `/recovery/EFI/recovery/initrd` | recovery netboot initrd copy on the recovery partition |
| `/recovery/EFI/BOOT/BOOT*.EFI` | recovery partition systemd-boot fallback |
| `/recovery/loader/entries/nixos-recovery.conf` | direct-firmware fallback menu entry |

Standard-Boot and Secure-Boot use the same loader entry; the secure path signs
the kernel and fallback bootloader with the configured db keys when they exist.
The recovery initrd is installed as a split initrd referenced by the signed
loader entry.
Updating the recovery image is a rebuild. Changing the recovery partition size
on an already-installed disk is a real partition resize operation, not something
this config does online.

Recovery boots include `nixos-mount`. Run it after SSH or local console access
to unlock the root LUKS device if needed, mount the installed system at `/mnt`,
mount persistent subvolumes that exist, and mount `/boot` plus `/recovery`:

```bash
nixos-mount
```

For an existing machine such as FRACTAL-NORTH, do this from a live environment
after backup and disk inspection. The desired end state is:

| Partition | Size | Label | Purpose |
|---|---:|---|---|
| ESP | 4G | `disk-main-boot` | normal systemd-boot/Lanzaboote boot partition |
| Root | remaining space | `disk-main-root` | existing LUKS/Btrfs root |
| Recovery | 4G | `disk-main-recovery` | recovery boot partition |

If there is already free space, creating recovery is straightforward:

```bash
sgdisk -n 0:0:+4G -t 0:EF00 -c 0:disk-main-recovery /dev/nvme0n1
mkfs.vfat -F 32 -n RECOVERY /dev/disk/by-partlabel/disk-main-recovery
```

If the existing ESP is too small, do not try to grow it in place when root sits
after it. Instead shrink Btrfs and the root partition offline, rename the old
ESP to `disk-main-boot-legacy`, create a new 4G `disk-main-boot` ESP near the
end of the disk, and create the 4G `disk-main-recovery` partition beside it.
After `/boot` and `/recovery` mount, rebuild or install from the mounted system;
the recovery service populates the recovery kernel, initrd, fallback bootloader,
and boot entry.

### Disk

`nix/system/disk.nix` owns the `disko` layout. The default model is:

| Layer | Default |
|---|---|
| Partition table | GPT |
| EFI partition | `settings.disk.boot.size` (`4G`), vfat, mounted at `settings.disk.boot.efiSysMountPoint` |
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
| `restore-generation` | Roll back to `settings.disk.immutability.restoreGeneration` |
| `disabled` | No reset |

`enforce.onReboot` wires the initrd reset + mounts service. Updates do not
rewrite the `CLEAN` baseline from the running root; the boot-time reset path is
the only automatic immutability reconciliation path.

The v2 mode persists selected files/directories by materializing persistent
btrfs subvolumes under `@persist` and bind-mounting selected targets after
reset. The source-of-truth persist list is
`settings.disk.immutability.persist.paths`.

Immutability state is versioned in `@snapshots/.immutability/version`. Missing
state means the legacy pre-0.1 layout. Version `0.1` uses numeric
`generation-N` rollback slots and reversible persist keys. On boot, any stored
version older than the current runtime version runs migrations before reset:
legacy `A`/`B`/`C` rollback slots become `generation-1`/`generation-2`/
`generation-3`, and unambiguous legacy persist keys are moved to the reversible
encoding.

`settings.disk.immutability.nonPersistedGenerations` controls how many reset
generations are retained. The default `3` preserves the old effective behavior;
`0` keeps no non-persisted rollback generations. Restore modes assert that the
configured retention count is high enough for the requested generation.

When a persisted path should become ordinary immutable state, edit the config
first, but run materialization before rebuilding into that new spec:

```bash
sudo nixosctl persistence materialize --refresh-clean
sudo nixosctl update
```

The command uses the currently installed `/etc/immutability/spec.toml`, copies
the still-persisted data back into its reset subvolume, and refreshes `CLEAN`.
The following rebuild then removes the path from the generated spec. Running the
rebuild first loses the old spec and leaves no declarative way to know what
should be copied back.

Common path transitions:

| User flow | Handling |
|---|---|
| Enable immutability with `n` paths | Rebuild; first reset creates or migrates `@persist` stores and mounts selected paths |
| Add a persisted path | Rebuild; reset creates the store from live or `CLEAN` data when it exists |
| Remove a persisted path | Edit config, run `nixosctl persistence materialize --refresh-clean`, rebuild; stale dir/file stores are quarantined on the next reset |
| Rename or move a persisted path | Treat as remove old + add new; materialize/checkpoint the old path before rebuilding |
| Disable immutability but keep persistence | Rebuild; reset stops, but `persistence-mounts` can still mount configured stores |
| Disable persistence or clear all paths | Materialize/checkpoint before rebuilding if data should remain in place |
| Upgrade immutability | Rebuild; version migration runs automatically during reset/materialize |
| A configured path no longer exists | `auto` resolves to file unless a trailing slash or `kind = "dir"` declares a directory; missing stores remain absent |
| Directory becomes file or file becomes directory | Resolved plans keep only the matching store kind; the old kind is quarantined as an orphan |

Orphans are reviewable state, not silent deletion. Directory stores move under
`@persist/.orphans/<key>-<stamp>` and file stores stay in their file-store
subvolume under `@persist/.immutability/files/.orphans/<stamp>/<key>`.

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

The app layer is recursively imported but policy-gated through
`settings.apps`. Category gates let lean profiles turn off whole groups:
`settings.apps.dev.enable`, `settings.apps.gaming.enable`, and
`settings.apps.utils.enable`.

| Path | Behavior |
|---|---|
| `dev/agents.nix` | Installs shared skills from `nix/apps/dev/agents/skills` under `~/.agents/skills` |
| `dev/bash.nix` | Adds `nix-shell-with-pkgs` helper |
| `dev/claude.nix` | Symlinks shared skills into Claude's `~/.claude/skills` tree |
| `dev/codex.nix` | Symlinks shared skills into Codex's `~/.codex/skills` tree without replacing built-in skills |
| `dev/git.nix` | Enables system Git; per-user identities live in Home Manager |
| `dev/vscode.nix` | Installs VSCode, the BTRC extension, and `btrc-lsp` from the flake-pinned BTRC package set |
| `gaming/battlenet/default.nix` | Installs a Battle.net Proton wrapper, desktop entry, and capture helper on x86_64 when Steam is enabled |
| `gaming/proton/default.nix` | Installs the patched Proton compatibility tool when Steam is enabled |
| `gaming/rocksmith/default.nix` | Installs Rocksmith support files and Slopsmith when Steam is enabled |
| `gaming/steam.nix` | Exports Steam tool names and configures installed Steam apps when Steam is enabled |
| `gaming/sunshine/default.nix` | Overlays the custom Sunshine package on x86_64 and sets runtime library paths when Sunshine is enabled |
| `utils/1password.nix` | Installs 1Password GUI/CLI and writes SSH agent config |
| `utils/nixosctl/nixosctl.nix` | Builds + packages the BTRC `nixosctl` CLI into `environment.systemPackages` |
| `utils/nixosctl/nixosctl-tray.nix` | Packages the BTRC tray GUI for `nixosctl` |
| `utils/rclone.nix` | Defines a user rclone mount for `~/Drive` |

The custom Proton tree under `nix/apps/gaming/proton/custom` is a
large patch-carrying workflow for Battle.net/Wayland/HDR behavior. Keep that
area focused and avoid broad formatting churn; its local README has the
development loop.

VFIO has been removed from this repo for now because the path was broken and
explicitly out of scope for the BTRC rewrite.

## BTRC Management Layer

The management tooling is BTRC at the repository root. The compiler, language
server, and VSCode extension are consumed through the flake input `inputs.btrc`,
pinned in `flake.lock` to `github:schiffy91/btrc`. The dev shell provides
`btrcpy` and `btrc-lsp`; local and Nix builds compile with `--strict-imports`.

| Path | Purpose |
|---|---|
| `btrc/nixosctl/nixosctl.btrc` | Main CLI (also hosts the VM e2e harness: `nixosctl graph` / `e2e`) |
| `btrc/nixosctl/tray.btrc` | Tray launcher for `nixosctl` |
| `btrc/nixosctl/btrfs` | Snapshot and diff helpers for `nixosctl` |
| `btrc/nixosctl/desktop` | Audio, display, caffeine, and hardware config commands |
| `btrc/nixosctl/hardware` | Secure Boot and TPM2 commands |
| `btrc/immutability/immutability.btrc` | BTRC entrypoint for the immutability runtime |
| `btrc/immutability/lib` | Immutability runtime library modules |
| `btrc/core` | Shared logging, path, and interactive helpers |
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

On installed hosts the CLI is packaged as `nixosctl` (see `nix/apps/utils/nixosctl/nixosctl.nix`)
and is on `PATH`. The native system tray (macOS menu bar / Linux Wayland
StatusNotifierItem) is implemented in the BTRC stdlib and packaged by
`nix/apps/utils/nixosctl/nixosctl-tray.nix`.

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
make -C tests qemu-host
make -C tests graph-coverage
make -C tests installer-download
make -C tests installer-ssh
make -C tests graph-full
```

Immutability/CLI scenario nodes: `immutability-reset`, `immutability-disabled`,
`immutability-snapshot-only`, `immutability-restore`, `immutability-orphan`,
`immutability-nonexistent-persist`, `immutability-key-encoding`,
`immutability-materialize`, `immutability-generations`,
`immutability-persist-owners`, `immutability-files`, `nixosctl-cli`,
`nixosctl-diff`.
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
nix eval .#nixosConfigurations.QEMU-aarch64-Standard-Boot.config.system.build.toplevel.drvPath --show-trace
nix eval .#nixosConfigurations.QEMU-x86_64-Standard-Boot.config.system.build.toplevel.drvPath --show-trace
make build
make -C tests graph-coverage
make -C tests app-settings
make -C tests qemu-host
```

For all flake outputs:

```bash
nix flake check --all-systems --no-build --show-trace
```

For disk-only targets, evaluate disk attributes directly:

```bash
nix eval .#diskoConfigurations.QEMU-aarch64-Disk-Operation.disko.devices.disk.main.device --show-trace
nix eval .#diskoConfigurations.QEMU-x86_64-Disk-Operation.disko.devices.disk.main.content.partitions.root.content.type --show-trace
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
| Native system tray | Implemented through the BTRC stdlib tray backend and packaged as `nixosctl-tray` |
| VFIO | Removed for now |
| Python / Rust | Removed (only an x86-gated Sunshine EDID packaging helper remains) |

## Maintenance Rules

1. Prefer `settings.*` for host policy.
2. Keep architecture-specific behavior as capability gates, not broad host-level removal.
3. Keep app installs under `settings.apps.*`.
4. Keep QEMU architecture assumptions inside the matching `nix/hosts/<arch>/QEMU`.
5. Do not add tests that only assert that broken or unimplemented behavior is absent.
6. Add positive tests for expected behavior.
7. Run at least the BTRC build and relevant graph node after touching BTRC or VM test code.
8. Do not reintroduce VFIO until there is a clean design and expected-behavior test path.
9. Do not remove desktop packages or host customizations while making cross-platform changes unless they are concretely unsupported on that platform.
