# Machine configuration

This repository configures NixOS and macOS machines in
`nix/hosts`, with local management tooling written in BTRC. System
management is Nix plus BTRC end to end; there is no Python or Rust in the
management layer.

## Current Hosts

| Host | System | Purpose |
|---|---|---|
| `FRACTAL-NORTH` | `x86_64-linux` | Main desktop/workstation configuration |
| `STEAM-DECK-OLED` | `x86_64-linux` | Steam Deck OLED on [Jovian-NixOS](https://github.com/Jovian-Experiments/Jovian-NixOS) |
| `QEMU` | `aarch64-linux` | Declarative ARM guest used for Nix evaluation and VM-oriented testing |
| `QEMU` | `x86_64-linux` | Declarative x86 guest used for Nix evaluation and VM-oriented testing |
| `mbp21` | `aarch64-darwin` | MacBook Pro |

Each Linux host is exposed with bootable NixOS targets and disk-operation disko
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
import `nix/system/nixos/disk.nix` directly from `flake.nix` and do not appear under
`nixosConfigurations`, so they avoid bootloader and user assertions that only
matter for real NixOS profiles.

## Repository Layout

| Path | Purpose |
|---|---|
| `flake.nix` | Host discovery, bootable targets, disko targets, and the pinned BTRC compiler input |
| `shell.nix` | Direnv-friendly dev shell with `btrcpy`, `btrc-lsp`, Nix tooling, Git, and a C toolchain |
| `nix/settings.nix` | Central typed settings interface shared across hosts and modules |
| `nix/system/nixos/` | NixOS system modules |
| `nix/system/darwin/` | nix-darwin modules applying Mac settings |
| `nix/home/` | Home Manager modules shared by Linux and the Mac; `plasma.nix` is Linux-only |
| `nix/apps/` | Globally imported app and helper modules; each optional app is settings-gated |
| `nix/apps/gaming/` | Custom package overrides for Proton, Rocksmith assets, Sunshine, and Battle.net helper tooling |
| `nix/hosts/x86_64/FRACTAL-NORTH/` | Main workstation host modules and host-specific data |
| `nix/hosts/x86_64/STEAM-DECK-OLED/` | Steam Deck OLED host on Jovian |
| `nix/hosts/aarch64/QEMU/` | ARM QEMU guest host modules |
| `nix/hosts/x86_64/QEMU/` | x86 QEMU guest host modules |
| `btrc/nixosctl/` | BTRC management CLI, tray entrypoint, and `nixosctl` feature modules |
| `btrc/immutability/` | BTRC immutability entrypoint and engine modules |
| `btrc/core/` | Shared BTRC helpers used by `nixosctl`, tests, and immutability |
| `build/` | Gitignored local compiler intermediates and binaries |
| `tests/` | Declarative VM e2e graph, per-scenario specs, and flake checks |
| `Makefile` | Root build/test contract (`make build`, `make check`, `make -C tests graph-*`) |

Linux host discovery is convention-based:

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
| `settings.boot.*` | Boot method, kernel, Plymouth, secure-boot PKI bundle and key re-enrollment, generation limit, timeout |
| `settings.sleep.*` | Hibernation, and whether suspend requests hibernate instead (implies hibernation) |
| `settings.tpm.*` | TPM2 device and version paths |
| `settings.desktop.*` | Default session, display outputs, cursor theme/package/path, Plasma theme values |
| `settings.apps.*` | Optional app policy flags |
| `settings.networking.*` | LAN subnet, firewall port intent, SSH identity agent, primary NIC |
| `settings.input.*` | Libinput mouse overrides |
| `settings.apps.rocksmith.*` | Audio buffer/sample rate used by Steam/Rocksmith and low-latency PipeWire config |
| `settings.apps.gaming.cpuAffinity` | Optional Linux CPU list for Steam games and Battle.net; `null` keeps normal scheduling |
| `settings.apps.nixosctl.*` | `nixosctl` config path, default-on idle inhibit |
| `settings.apps.sunshine.*` | Capture backend (`kms` or `kwin`), CUDA build, KWin virtual display |
| `settings.sudolessAllowlist.*` | Optional sudo NOPASSWD command/package allowlist |

### Gaming CPU affinity and power profiles

`settings.apps.gaming.cpuAffinity = "0-7,16-23";` starts managed Steam games
and Battle.net on the first CCD of FRACTAL-NORTH's Ryzen 7950X,
including its SMT threads. CPU numbering is host-specific; other hosts default
to `null`. Steam's per-game launch options remain intact inside the wrapper.
Steam must be closed during activation so its configuration can be updated.
Restart Battle.net and any running games to inherit the new affinity.

`game-run COMMAND ...` applies the same policy to other launchers. Set
`GAME_CPU_AFFINITY=all` before the wrapper to skip applying the policy for one
launch, or set the Nix option to `null` and rebuild to disable it globally.
This sets inherited scheduling affinity; applications can reset their own
thread affinity. It does not reserve those cores or promise higher frame rates.

The nixosctl tray's **CPU** submenu lists the power profiles advertised by power-profiles-daemon
and marks the active one. Selecting Performance, Balanced, or Power Saver
changes the profile immediately through the normal desktop authorization.
Unavailable profiles are omitted. The CLI equivalents are `nixosctl power
list`, `nixosctl power current`, and `nixosctl power set performance`.
`nixosctl power is performance` returns success only while that profile is
active. These controls do not change GPU power limits or clocks.

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
`settings.apps.steam.enable`, and an x86_64 host platform. Sunshine's CUDA
build is opt-in through `settings.apps.sunshine.cuda.enable`; other hosts keep
upstream `pkgs.sunshine`.

Plasma desktop state is intentionally not treated as an optional app flag. It is
part of the shared desktop surface, not an app install.

## System Modules

### Boot

`nix/system/nixos/boot.nix` selects behavior from `settings.boot.method`.

| Method | Behavior |
|---|---|
| `Standard-Boot` | Enables systemd-boot |
| `Secure-Boot` | Disables systemd-boot directly and enables Lanzaboote |
| `Disk-Operation` | Not handled by this module; the flake exposes `nix/system/nixos/disk.nix` through `diskoConfigurations` |

The shared boot module sets current kernel packages, Plymouth, quiet boot
parameters, the EFI mount point, and `settings.boot.previousGenerationLimit` as
the loader's `configurationLimit`. The initrd is systemd's — scripted stage 1
no longer exists in nixpkgs — and the module only orders
`plymouth-start.service` after `systemd-modules-load.service` inside it.

#### Standard-Boot Safety Guard

`nix/system/nixos/boot/safety.nix` refuses to write unsigned boot files while
the firmware has Secure Boot enabled. It reads
`SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c` from `efivarfs` and fails
activation when the byte says enabled, and equally when the state cannot be
determined — an unmounted `efivarfs` is not treated as "no Secure Boot". The
guard runs in two places, because they are two different bypasses:

| Hook | Covers |
|---|---|
| `system.preSwitchChecks.secureBoot` | `nixos-rebuild switch` on a running system |
| `system.build.installBootLoader` wrapper | `nixos-install` and `NIXOS_NO_CHECK`, before the first ESP write |

It is active only when `settings.boot.method == "Standard-Boot"`; the guard
itself is also exposed as the `bootSafety` module argument. Switching a
Secure-Boot machine back to Standard-Boot therefore requires disabling Secure
Boot in firmware first, which is deliberate: the alternative is an ESP full of
unsigned files that the firmware will not boot.

### Recovery

`settings.disk.recovery.enable = true` adds a `RECOVERY` vfat partition to the
destructive disko layout and enables the `nixos-recovery-boot-entry` service on
installed systems. The recovery image is not a downloaded ISO; it is a
self-contained NixOS netboot environment built from `nix/system/nixos/recovery.nix`.
Secure-Boot embeds it in a signed unified kernel image.

On switch, the service requires `/boot` and `/recovery` to be mounted, then
installs:

| Path | Purpose |
|---|---|
| `/boot/EFI/recovery/nixos-recovery.efi` | signed unified recovery image in Secure-Boot |
| `/boot/EFI/recovery/kernel.efi`, `initrd` | split recovery payload in Standard-Boot |
| `/boot/loader/entries/nixos-recovery.conf` | systemd-boot/Lanzaboote menu entry |
| `/recovery/EFI/recovery/nixos-recovery.efi` | independently bootable signed recovery image |
| `/recovery/EFI/recovery/kernel.efi`, `initrd` | split recovery payload in Standard-Boot |
| `/recovery/EFI/BOOT/BOOT*.EFI` | recovery partition systemd-boot fallback |
| `/recovery/loader/entries/nixos-recovery.conf` | direct-firmware fallback menu entry |

Standard-Boot uses a split kernel/initrd. Secure-Boot uses a signed unified
image that authenticates the kernel, initrd and command line together. It also
signs the fallback loader with the configured db key. Installed checksums and
signatures are checked before skipping an update; files are staged before
replacement. Missing mounts fail the recovery service.
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

`nix/system/nixos/disk.nix` owns the `disko` layout. The default model is:

| Layer | Default |
|---|---|
| Partition table | GPT |
| EFI partition | `settings.disk.boot.size` (`4G`), vfat, mounted at `settings.disk.boot.efiSysMountPoint` |
| Root partition | btrfs, optionally wrapped in LUKS |
| Subvolumes | `@root`, `@home`, `@nix`, `@var`, `@snapshots`, `@swap` |

The QEMU host overrides the disk to `/dev/vda`, disables encryption, enables
the immutability reset workflow, and sets a small swap size suitable for VM use.

### Immutability

`nix/system/nixos/immutability.nix` drives the BTRC **immutability**, the
sole immutability implementation. Its source entrypoint is
`btrc/immutability/Immutability.btrc`; Nix builds it with the flake-locked
BTRC compiler and a generated stdlib archive.

The initrd reset runs after `systemd-hibernate-resume.service`. A successful
resume never returns to the initrd, so the reset only runs on a fresh boot and
never rewrites the filesystem underneath a hibernated kernel. That ordering is
the whole reason hibernation is safe on an immutable root, so it is asserted in
`app-settings` for every host that hibernates.

Resume itself needs no kernel command line, even with the swap file on btrfs:
`systemd-hibernate` writes the device and offset into the `HibernateLocation`
EFI variable and the initrd generator reads it back, so `boot.resumeDevice`
stays empty. See the Steam Deck README for why adding `resume=` by hand makes
this worse rather than better.

| Mode | Behavior |
|---|---|
| `converge` | **Default.** Same rollback as `reset`; the two names are interchangeable in `Immutability.resetMode()` |
| `reset` | Roll each `resetOnBoot` subvolume back to its read-only `CLEAN` snapshot at boot |
| `prepare-only` | Build `NEXT` from `CLEAN` and persisted state without publishing it |
| `publish-prepared` | Publish an existing prepared `NEXT` at boot without rebuilding it |
| `snapshot-only` | Rotate snapshots without rolling back (ephemeral survives) |
| `restore-previous` / `restore-a` | Roll back to generation 1 |
| `restore-penultimate` / `restore-b` | Roll back to generation 2 |
| `restore-c` | Roll back to generation 3 |
| `restore-generation` | Roll back to `settings.disk.immutability.restoreGeneration` |
| `disabled` | No reset |

Any other value is fatal in the initrd (`Unknown mode:`), so the enum in
`nix/settings.nix` and `ImmutabilityConstants` must stay in step.

`enforce.onReboot` wires the initrd reset + mounts service. Updates do not
rewrite the `CLEAN` baseline from the running root; the boot-time reset path is
the only automatic immutability reconciliation path.

For a more inspectable reset, run the live preparation path before switching the
boot mode:

```bash
sudo nixosctl immutability plan
sudo nixosctl immutability prepare
```

`plan` prints the resolved keep plan from the installed config. `prepare` mounts
the btrfs top level, rotates generations, and builds `NEXT` while the current
system keeps running. Setting
`settings.disk.immutability.mode = "publish-prepared"` then makes initrd only
verify and publish that prepared snapshot. Runtime logs are also written under
`@snapshots/.immutability/logs/`.

The v2 mode persists selected files/directories by materializing persistent
btrfs subvolumes under `@persist` and bind-mounting selected targets after
reset. The source-of-truth persist list is
`settings.disk.immutability.persist.paths`.

Persist stores use reversible keys: the absolute path without its leading slash
is URL-escaped, then encoded path separators are written as `!`. For example,
`/home/alex/name with spaces` becomes `home!alex!name%20with%20spaces`.
Persisted directories live under `@persist/dirs`; persisted files live under
`@persist/.immutability/files`.

The reset path records the active immutability state version in
`@snapshots/.immutability/version`, mounted at
`/.snapshots/.immutability/version` after boot. The current version is
`2026.08.22`; any future version-specific transition belongs in
`nix/system/nixos/immutability.nix`.

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
| Enable immutability with `n` paths | Rebuild; first reset creates canonical `@persist` stores and mounts selected paths |
| Add a persisted path | Rebuild; reset creates the store from live or `CLEAN` data when it exists |
| Remove a persisted path | Edit config, run `nixosctl persistence materialize --refresh-clean`, rebuild; stale dir/file stores are quarantined on the next reset |
| Rename or move a persisted path | Treat as remove old + add new; materialize/checkpoint the old path before rebuilding |
| Disable immutability but keep persistence | Rebuild; reset stops, but `persistence-mounts` can still mount configured stores |
| Disable persistence or clear all paths | Materialize/checkpoint before rebuilding if data should remain in place |
| Upgrade immutability | Rebuild; persist keys are canonical-only in this tree |
| A configured path no longer exists | `auto` resolves to file unless a trailing slash or `kind = "dir"` declares a directory; missing stores remain absent |
| Directory becomes file or file becomes directory | Resolved plans keep only the matching store kind; the old kind is quarantined as an orphan |

Orphans are reviewable state, not silent deletion. Directory stores move under
`@persist/.orphans/<key>-<stamp>` and file stores stay in their file-store
subvolume under `@persist/.immutability/files/.orphans/<stamp>/<key>`.

### Desktop

`nix/system/nixos/desktop.nix` enables the Plasma 6 Wayland desktop, SDDM, dconf,
graphics, cursor packages, and selected KDE integrations. The display manager
starts `settings.desktop.defaultSession`.

Host-specific desktop state lives in the host modules:

| File | Role |
|---|---|
| `nix/hosts/x86_64/FRACTAL-NORTH/gpu.nix` | NVIDIA/AMD graphics setup, device symlinks, KWin DRM devices |
| `nix/hosts/x86_64/FRACTAL-NORTH/audio.nix` | Low-latency PipeWire and device-specific audio rules |
| `nix/hosts/x86_64/FRACTAL-NORTH/input.nix` | Controller and mouse receiver support |

### Networking And Security

`nix/system/nixos/networking.nix` owns NetworkManager, firewall rules, mDNS,
OpenSSH, SSH agent forwarding, and optional primary-interface enforcement.
Firewall LAN allow rules are derived from shared service flags and
`settings.networking.ports.{tcp,udp}`.

`nix/system/nixos/users.nix` imports Home Manager and every top-level app module
under `nix/apps`. It defines immutable users: the interactive admin account and
an isolated agent account with its own Home Manager profile and tool set. User
mutability is disabled, so password changes must update the hashed password file
and rebuild.

`nix/system/nixos/sudolessAllowlist.nix` is opt-in through
`settings.sudolessAllowlist.enable`.

### Secure Boot

`settings.boot.method = "Secure-Boot"` disables systemd-boot and hands the ESP
to Lanzaboote, which signs the unified kernel images with the PKI bundle at
`boot.lanzaboote.pkiBundle` (`/var/lib/sbctl`). `boot.lanzaboote.settings.editor`
is false, so the loader offers no interactive command line to edit.

Enrollment and teardown go through `nixosctl`, never through raw `sbctl`:

```bash
sudo nixosctl secure-boot status           # sbctl status + verify our own images
sudo nixosctl secure-boot enable           # our keys only
sudo nixosctl secure-boot disable
```

| Step in `secure-boot enable` | Why it is in that order |
|---|---|
| Require firmware Setup Mode | Enrollment cannot replace keys on an already-enrolled machine; use `nixosctl update` there |
| `sbctl create-keys` and assert a complete PK/KEK/db bundle | A partial bundle would enroll a machine we cannot sign for |
| Switch the recorded target and rebuild | The ESP must hold signed files *before* the firmware starts enforcing |
| Verify, then enroll | `requireVerified` is fatal on any unsigned image, so enrollment never runs against an ESP we have not proven |
| Prompt for reboot | Enforcement only begins on the next boot |

Verification covers the fallback loader, `systemd-boot<arch>.efi`, the default
boot entry resolved from `bootctl --json=short list`, and every
`EFI/Linux/*.efi`. When `settings.disk.recovery.enable` is set it additionally
checks the recovery UKI on both the ESP and the recovery partition, plus the
recovery partition's fallback loader, with `sbverify --cert` against our db
certificate.

**Never enroll Microsoft's keys.** `secure-boot enable` takes a `--microsoft`
flag, and it is not used on any host. Without it, enrollment passes
`--yes-this-might-brick-my-machine` and trusts our keys alone. The cost is
real: option ROMs and vendor firmware updaters that ship Microsoft-signed
binaries are not trusted, including the Steam Deck's `jupiter-biosupdate`. The
answer there is to disable Secure Boot in firmware for that one operation, not
to widen the trust store. Do not reach for `--microsoft` to unblock something
else.

`secure-boot disable` is the mirror image: it refuses while the firmware still
enforces Secure Boot (`Disable Secure Boot in firmware before installing
Standard-Boot`), which is the same invariant the Standard-Boot safety guard
enforces at activation time.

### Firmware Security Posture

`fwupdmgr security` is the health check for the platform side of security.
`FRACTAL-NORTH` sits at **HSI:1**, which is the ceiling for a consumer AM5
board: HSI-2 needs AMD Platform Secure Boot, an OEM fuse ASUS does not burn.
The required BIOS settings, the expected report, and the re-check routine after
every firmware flash live in
[`nix/hosts/x86_64/FRACTAL-NORTH/BIOS.md`](nix/hosts/x86_64/FRACTAL-NORTH/BIOS.md).

Two facts are easy to forget:

1. ASUS resets AMD CBS settings on every BIOS flash. Secure Boot and TSME both
   dropped silently on 2026-05-18 that way. Re-run the checklist after flashing.
2. The board's ACPI never marks the Thunderbolt 4 root port `ExternalFacingPort`,
   so the kernel cannot flag tunneled devices as untrusted. `cpu.nix` therefore
   leaves the IOMMU in translated mode (no `iommu=pt`); that is the only DMA
   protection Thunderbolt gets on this hardware.

## Apps And Package Overrides

The app layer is recursively imported but policy-gated through
`settings.apps`. Category gates let lean profiles turn off whole groups:
`settings.apps.dev.enable`, `settings.apps.gaming.enable`, and
`settings.apps.utils.enable`.

| Path | Behavior |
|---|---|
| `dev/zsh.nix` | Makes zsh the admin login shell when `settings.apps.zsh.enable` is set, leaving compinit to Home Manager |
| `dev/git.nix` | Enables system Git; per-user identities live in Home Manager |
| `dev/vscode.nix` | Installs VSCode, the BTRC extension, and `btrc-lsp` from the flake-locked BTRC package set |
| `gaming/battlenet/default.nix` | Installs a Battle.net Proton wrapper, desktop entry, and capture helper on x86_64 when Steam is enabled |
| `gaming/proton/default.nix` | Installs the patched Proton compatibility tool when `settings.apps.steam.protonCustom.enable` is set |
| `gaming/rocksmith/default.nix` | Installs Rocksmith support files and Slopsmith when Steam is enabled |
| `gaming/steam/default.nix` | Exports Steam tool names, configures installed Steam apps, and installs `settings.apps.steam.shortcuts` as non-Steam shortcuts when Steam is enabled |
| `gaming/sunshine/default.nix` | Sunshine service and ports; the CUDA overlay, and the KWin HDR screencast patch when capture is `kwin` |
| `gaming/sunshine/virtual-display.nix` | KWin virtual streaming display and its Desktop/Big Picture apps when `virtualDisplay.enable` is set |
| `gaming/assettocorsa/default.nix` | Assetto Corsa content import, CM presets, control bindings, UI scale, and the CSP wheel menu |
| `gaming/simracing/default.nix` | Enables the `logitech-trueforce` kernel module on x86_64 for wheel force feedback |
| `gaming/semu/default.nix` | Installs `semu` from its flake input, sets `SEMU_TARGET`, writes the paths override, and runs the save-sync user service |
| `gaming/btrsmith.nix` | Installs BTRSmith when an exact flake input provides it |
| `gaming/proton/dpi.nix` | `wine-prefix-dpi`, which writes `LogPixels` into a prefix registry |
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
server, and VSCode extension are consumed through the flake input `inputs.btrc`.
The dev shell provides `btrcpy` and `btrc-lsp`; local and Nix builds compile
with `--strict-imports`.

That input tracks upstream's default branch; `flake.lock` records the revision
in use and `nix flake update btrc` moves it.

| Path | Purpose |
|---|---|
| `btrc/nixosctl/NixosCtl.btrc` | Main CLI (also hosts the VM e2e harness: `nixosctl graph` / `e2e`) |
| `btrc/nixosctl/NixosTray.btrc` | Tray launcher for `nixosctl` |
| `btrc/nixosctl/btrfs` | Snapshot and diff helpers for `nixosctl` |
| `btrc/nixosctl/desktop` | Audio, display, caffeine, and hardware config commands |
| `btrc/nixosctl/hardware` | Secure Boot (`SecureBootManager` + the `SecureBootVerification` parsers) and TPM2 commands |
| `btrc/immutability/Immutability.btrc` | BTRC entrypoint for the immutability runtime |
| `btrc/immutability/lib` | Immutability runtime library modules |
| `btrc/core` | Shared logging, path, and interactive helpers |
| `btrc/gaming` | Steam, Wine prefix and Assetto Corsa tools built by `nix/apps/gaming/package.nix` |
| `tests/e2e` | Declarative VM graph and test runner |

Every module filename is PascalCase and matches its primary class, and the
`btrc/` tree is the `SOURCES` set the Makefile hashes, so renaming a module
invalidates every recorded VM state.

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

`tests/graph.json` is the authoritative node list: its `required` set names
every node that guards a contract, and `make -C tests graph-status` prints each
one with its state (`ready`, `stale`, `missing`) rather than repeating the list
here. The families are `installer-*`, `install-*`, `immutability-*`,
`recovery-*`, `secure-boot-*`, `tpm2-*`, `nixosctl-*`, `password-change*`,
`qemu-host-*`, `qmp-probe` and the `spec-toml`/`spec-yaml` format checks.

The nodes with constraints worth knowing before you run them:

- `tpm2-enroll` enrolls and wipes a TPM2 LUKS keyslot on an encrypted install.
- `secure-boot-aarch64` runs the full Lanzaboote flow — install, sign,
  auto-enroll keys, then `bootctl status` → `Secure Boot: enabled`. It needs
  enforcing firmware, so `tests/shell.nix` builds Debian's
  `AAVMF_CODE.secboot.fd` in a reproducible derivation. `addAccelerator` picks
  HVF on a same-arch Mac, KVM on same-arch Linux, and `tcg,thread=multi`
  otherwise — so this node is accelerated on the Mac and emulated (slow) on
  the x86_64 workstation.
- `install-nixosctl` runs the documented `make build` and
  `nixosctl install --format` inside the installer ISO. The live ISO keeps its
  Nix store in RAM, so the node uses 16G, the Deck's memory.
- `immutability-hibernate` hibernates and suspends-to-disk an encrypted install
  and checks that the initrd reset never runs before a resume. It declares no
  `arch`, so it runs on the host's architecture. Its guest has one vCPU because
  QEMU's HVF backend crashes when hibernation offlines secondary CPUs.
- `recovery-secure-boot-*` boots the signed recovery UKI, repairs a damaged
  fallback loader, and reinstalls from recovery.

Encrypted boots use the `unlock-luks` op: it waits for the LUKS prompt on the
serial console, types the passphrase, then waits for SSH. Write negative guest
checks as `if cmd; then exit 1; fi`, because a bare `! pipeline` never trips
`set -e`.

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
for host in FRACTAL-NORTH-Standard-Boot FRACTAL-NORTH-Secure-Boot \
            STEAM-DECK-OLED-Standard-Boot STEAM-DECK-OLED-Secure-Boot \
            QEMU-x86_64-Standard-Boot QEMU-aarch64-Standard-Boot; do
  nix eval ".#nixosConfigurations.$host.config.system.build.toplevel.drvPath" --show-trace
done
nix eval .#darwinConfigurations.mbp21.config.system.build.toplevel.drvPath --show-trace
nix eval '.#homeConfigurations."alexanderschiffhauer@mbp21".activationPackage.drvPath' --show-trace
make build unit
make -C tests gaming
make -C tests graph-coverage
make -C tests app-settings
make -C tests qemu-host
```

Evaluating every bootable target matters after an input bump: a removed NixOS
option fails only on the hosts that set it, and the darwin and Home Manager
outputs do not share the Linux `nixpkgs`.

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

## Binary Cache And Releases

`ci/targets.nix` is the single list of what gets built, for which systems, on which
runners, and what a release carries. Adding a system or an attribute there is the whole
change: the Makefile targets, `ci/publish.py` and both GitHub workflows read that file.

One machine builds for the others, with no account anywhere:

```sh
make cache        # this host's ci/targets.nix attrs, signed into ~/.cache/nix-cache
make cache-all    # the `local` list instead: every host's system closure
make cache-serve  # serve that directory over HTTP (CACHE_PORT=8080)
```

The first run writes `~/.config/nix/nixos-cache-1.key` and prints its public half, which
is what the other hosts trust. The directory is shared with the other repositories on
the host, so one server covers them all. A consumer sets two options:

```nix
settings.nix.substituters = [ "http://FRACTAL-NORTH.local:8080" ];
settings.nix.trustedPublicKeys = [ "nixos-cache-1:..." "semu-cache-1:..." ];
```

`nix/system/nixos/nix.nix` applies them, with `connect-timeout = 5` and `fallback` so a
publisher that is switched off costs a build five seconds instead of failing it.
STEAM-DECK-OLED is wired this way already: the desktop builds its system and its
emulators, and the handheld activates instead of compiling them.

`.github/workflows/cache.yml` builds one job per system and attribute every Monday and
on every push to `main`, on x86_64 and aarch64 runners. With a `CACHIX_AUTH_TOKEN`
secret it pushes to the Cachix cache named in `ci/targets.nix`; with a
`NIX_CACHE_SECRET_KEY` secret and no Cachix it signs the closure and leaves a flat cache
as a workflow artifact, which `nix copy --from file://...` consumes unchanged. The
`NIXOS_CACHE_URL` and `NIXOS_CACHE_PUBLIC_KEY` repository variables let the runners
substitute from the cache they are filling.

Architectures split by who can build them. This host builds `x86_64-linux` only: it has
no `boot.binfmt.emulatedSystems`, so `nix` reports no aarch64 in `extra-platforms` and
`make cache` covers x86_64. `aarch64-linux` comes from CI's `ubuntu-24.04-arm` runner,
and `aarch64-darwin` from the Mac's own `make cache`. If you would rather build arm64
here, `boot.binfmt.emulatedSystems = [ "aarch64-linux" ]` enables it through QEMU, at
emulation speed and at the cost of a larger system closure.

`.github/workflows/release.yml` turns a `v*` tag into a GitHub Release carrying the
installer ISOs in `ci/targets.nix` `release`, each with its digest in `SHA256SUMS`. That
list is only the generic `installer`, which measured 1.9 GB against GitHub's 2 GiB asset
limit. A host installer carries that host's whole system closure *and every flake
input source*, so the machine installs with no network and can still evaluate its own
configuration afterwards; it measured 13 GB for FRACTAL-NORTH and 11 GB for
STEAM-DECK-OLED — past the asset limit and past a hosted runner's disk, so those are in
`local` and travel through `make cache` instead. The workflow still builds and frees one ISO at a time.

## Secrets And Local State

Ignored local paths:

| Path | Purpose |
|---|---|
| `secrets/` | Local secret material such as hashed password files |
| `config.json` | Local selected host/target for helper tooling |
| `.vm/` | QEMU disks, ISO downloads, state graph data |
| `.direnv/`, `.btrc-cache/`, `build/` | Development caches and build outputs |
| `nix/apps/gaming/proton/custom/src/` | Working Wine tree for the proton-custom dev loop (`make shell` / `make setup`) |
| `nix/apps/gaming/battlenet/tests/.build/` | Battle.net test scratch output |
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
| Python / Rust | Removed from the management layer |

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

## macOS

The Mac configuration is in [mbp21](nix/hosts/aarch64/mbp21/README.md).
Its `darwin.nix` is wired explicitly in `flake.nix`; Linux host discovery and
`nixosctl` retain their existing convention. `nix/settings.nix` declares the
package, shell, Git, agent and security policy. The Mac host supplies its
existing packages and shell configuration.

The two platforms pin separately on purpose. `nixosctl update --upgrade` passes
only the inputs whose names do not contain `darwin` (`NixosConfig.linuxInputs`
reads them out of `flake.lock`), and the Mac's `nix-upgrade` alias updates only
`nixpkgs-darwin`, `home-manager-darwin` and `nix-darwin`. A bare
`nix flake update` ignores that split and moves both sides at once, so use the
scoped commands unless you mean to bump everything.

```sh
nix build .#darwinConfigurations.mbp21.system --no-link
sudo darwin-rebuild switch --flake .#mbp21
```
