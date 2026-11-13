# Machine configuration

This repository configures NixOS and macOS machines in
`config/hosts`, with local management tooling written in BTRC. System
management, application tooling, CI orchestration and test scenarios use Nix
and BTRC. Native bindings handle operating-system and library boundaries.
Upstream build tools, including the pinned BTRC compiler and Wine generators,
may still depend on Python.

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
use `config/outputs/diskOperations.nix` to apply the resolved host disk settings
to `config/system/nixos/disk.nix`. They do not import unrelated host services or
appear under `nixosConfigurations`.

## Repository Layout

| Path | Purpose |
|---|---|
| `flake.nix` | Pinned inputs and the entry point into `config/outputs/` |
| `config/outputs/` | Host registry, system and home configurations, disk operations, installers, and checks |
| `config/nixos.nix`, `config/darwin.nix`, `config/home.nix` | Explicit platform and Home Manager module composition |
| `shell.nix` | Direnv-friendly dev shell with `btrcpy`, `btrc-lsp`, Nix tooling, Git, and a C toolchain |
| `config/settings.nix` | Entry point importing the system and package option catalogs |
| `config/system/options.nix` | Shared core options and imports for domain-owned system schemas |
| `config/system/nixos/` | NixOS system modules |
| `config/system/darwin/` | nix-darwin modules applying Mac settings |
| `config/pkgs/de/kde/home.nix` | Linux Plasma Home Manager integration |
| `config/pkgs/dev/` | Shared development tools, shell/Git Home Manager settings, and agent configuration |
| `config/pkgs/` | Explicit development, desktop environment, gaming, and utility module catalogs |
| `config/pkgs/gaming/` | Game modules, runtime source, packages and assets colocated by feature |
| `config/hosts/x86_64/FRACTAL-NORTH/` | Main workstation host modules and host-specific data |
| `config/hosts/x86_64/STEAM-DECK-OLED/` | Steam Deck OLED host on Jovian |
| `config/hosts/aarch64/QEMU/` | ARM QEMU guest host modules |
| `config/hosts/x86_64/QEMU/` | x86 QEMU guest host modules |
| `config/pkgs/utils/nixosctl/` | Management CLI, `lib/` runtime implementation, and `ui/` tray application |
| `config/system/storage/immutability/src/` | BTRC immutability entrypoint and engine modules |
| `config/pkgs/utils/nixosctl/lib/core/` | Shared BTRC helpers used by `nixosctl`, tests, and immutability |
| `build/` | Gitignored local compiler intermediates and binaries |
| `tests/` | Declarative VM e2e graph, per-scenario specs, and flake checks |
| `Makefile` | Root build/test contract (`make build`, `make check`, `make -C tests graph-*`) |
| `ci/` | Cache publishing and CI target definitions |
| `.github/workflows/` | GitHub-required workflow entry points calling `ci/` |

Host discovery checks exactly two directory levels:

```text
config/hosts/<architecture>/<HOST>/default.nix
```

`config/outputs/hosts.nix` records platform exceptions such as `mbp21` on Darwin.
`lib.hosts` exposes only physical Linux hosts to the installer picker; test adapters
under `tests/hosts` still receive system and installer outputs. Duplicate QEMU
names retain their architecture suffixes. Existing saved host paths are resolved
to the new entry locations by `nixosctl`.

## Settings Model

`config/settings.nix` composes `config/system/options.nix` and
`config/pkgs/options.nix`. Option definitions live beside their owning features:
users, storage, immutability, boot, sleep, networking, macOS, and individual packages.
The existing `settings.*` names remain the stable configuration interface. Host
modules should prefer overriding these options instead of directly changing
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
| `settings.apps.gaming.rocksmith.*` | Audio buffer/sample rate used by Steam/Rocksmith and low-latency PipeWire config |
| `settings.apps.gaming.cpuAffinity` | Optional Linux CPU list for Steam games and Battle.net; `null` keeps normal scheduling |
| `settings.apps.utils.nixosctl.*` | Structured display/audio settings, generated TOML config path, default-on idle inhibit |
| `settings.apps.gaming.sunshine.*` | Capture backend (`kms` or `kwin`), CUDA build, KWin virtual display |
| `settings.networking.forwardAgentHosts` | Explicitly trusted SSH agent-forwarding destinations |

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

Application settings follow their category: `apps.dev.vscode`,
`apps.gaming.steam`, and `apps.utils.chrome`. Modules check only their own
`enable` switch. Nix validates the hierarchy before building or activating it;
there is no runtime enablement resolver.

Default-on children inherit their parent's enablement. Optional tools such as
LM Studio and Gemini default off. An explicit `true` under a disabled parent
is an error, including `mkForce true`; either disable that child or enable its
parent. Non-enable configuration (URLs, paths, preferences) may remain declared
while an app is disabled. For example, this is rejected:

```nix
settings.apps.dev.enable = false;
settings.apps.dev.vscode.enable = true;
```

The error names both settings. Disabling a category in a host that explicitly
enables children requires changing those child settings too; it does not
silently override the host's declared intent. Common defaults are:

```nix
settings.apps.enable = true;
settings.apps.dev.enable = true;
settings.apps.gaming.enable = true;
settings.apps.utils.enable = true;
settings.apps.dev.agents.enable = true;
settings.apps.utils.onePassword.enable = true;
settings.apps.dev.bash.enable = true;
settings.apps.gaming.battlenet.enable = true;
settings.apps.dev.claude.enable = true;
settings.apps.dev.codex.enable = true;
settings.apps.dev.git.enable = true;
settings.apps.utils.nixosctl.enable = true;
settings.apps.utils.rclone.enable = true;
settings.apps.gaming.rocksmith.enable = true;
settings.apps.gaming.steam.enable = true;
settings.apps.gaming.sunshine.enable = true;
settings.apps.dev.vscode.enable = true;
```

Sibling dependencies are checked during evaluation too. Assetto Corsa,
Battle.net, and Rocksmith require `settings.apps.gaming.steam.protonCustom.enable`
and x86_64 Linux; WoW requires Battle.net. Their defaults follow those dependencies.
Sunshine's CUDA
build is opt-in through `settings.apps.gaming.sunshine.cuda.enable`; other hosts keep
the non-CUDA build. Both builds require provisioned administration credentials.

Chrome PWAs have individual switches and stable taskbar references:

```nix
settings.apps.utils.chrome.webApps.mail = {
  enable = true;
  name = "Mail";
  url = "https://mail.example.com/";
};
# Taskbar reference: config.settings.apps.utils.chrome.webApps.mail.launcher
```

Omitting a PWA's `enable` makes it follow Chrome. Setting it to `false` removes
its installation policy and managed launcher. Enabled PWAs under disabled
Chrome are rejected just like other children. The validator discovers enable
options from the module schema, so newly declared apps need no separate registry.

Plasma desktop state is intentionally not treated as an optional app flag. It is
part of the shared desktop surface, not an app install.

## System Modules

### Boot

`config/system/nixos/boot.nix` selects behavior from `settings.boot.method`.

| Method | Behavior |
|---|---|
| `Standard-Boot` | Enables systemd-boot |
| `Secure-Boot` | Disables systemd-boot directly and enables Lanzaboote |
| `Disk-Operation` | Not handled by this module; the flake exposes `config/system/nixos/disk.nix` through `diskoConfigurations` |

The shared boot module sets current kernel packages, Plymouth, quiet boot
parameters, the EFI mount point, and `settings.boot.previousGenerationLimit` as
the loader's `configurationLimit`. The initrd is systemd's — scripted stage 1
no longer exists in nixpkgs — and the module only orders
`plymouth-start.service` after `systemd-modules-load.service` inside it.

#### Standard-Boot Safety Guard

`config/system/nixos/boot/safety.nix` refuses to write unsigned boot files while
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
self-contained NixOS netboot environment built from `config/system/nixos/recovery.nix`.
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

`config/system/nixos/disk.nix` owns the `disko` layout. The default model is:

| Layer | Default |
|---|---|
| Partition table | GPT |
| EFI partition | `settings.disk.boot.size` (`4G`), vfat, mounted at `settings.disk.boot.efiSysMountPoint` |
| Root partition | btrfs, optionally wrapped in LUKS |
| Subvolumes | `@root`, `@home`, `@nix`, `@var`, `@snapshots`, `@swap` |

The QEMU host overrides the disk to `/dev/vda`, disables encryption, enables
the immutability reset workflow, and sets a small swap size suitable for VM use.

Optional boot-passphrase reuse is configured with
`settings.disk.encryption.opportunisticUnlock.enable`. It discovers attached
software LUKS volumes across storage transports, skips root and already-open
backing devices, and unlocks matching volumes without mounting them. The verified
passphrase is consumed from a short-lived kernel key; TPM-only/keyfile boots and
later hotplug do not trigger attempts. See the [policy and boot regression
gates](tests/opportunistic-unlock/README.md) for eligibility and resource limits.

### Immutability

`config/system/storage/immutability/default.nix` drives the BTRC **immutability**, the
sole immutability implementation. Its source entrypoint is
`config/system/storage/immutability/src/Immutability.btrc`; Nix builds it with the flake-locked
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
`config/system/storage/immutability/options.nix` and `ImmutabilityConstants` must stay in step.

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

This option starts with an empty list. System modules contribute core state, and
enabled package modules contribute their own state through ordinary Nix list
definitions. The merged result removes exact duplicates. Shared desktop state
belongs to the desktop module, so disabling Chrome or Sunshine does not remove
shared icon, application, or desktop state. Persistence contributions remain
available when `settings.disk.immutability.enable = false`; the separate
`settings.disk.persistence.enable` flag controls persistent mounting.

For example, Sunshine contributes its configuration directory inside its enabled
module. A host can add paths in the same way, including state it wants to retain
while a package is disabled:

```nix
settings.apps.gaming.sunshine.enable = false;
settings.disk.immutability.persist.paths = [
  "${config.settings.users.admin.homeDirectory}/.config/sunshine"
];
```

Package contributions follow their app and group switches. Turning a feature off
removes its paths from the active specification unless another module retains
them. The next prepare/reset quarantines unused stores; re-enabling the feature
does not automatically recover those stores. Keep an explicit host contribution
for temporary disablement, or use the materialization workflow below when
retiring persistence. The `persistence-ownership` and `immutability-feature-disable`
graph checks cover merging, feature switches, quarantine, recovery, and retention.

Persist stores use reversible keys: the absolute path without its leading slash
is URL-escaped, then encoded path separators are written as `!`. For example,
`/home/alex/name with spaces` becomes `home!alex!name%20with%20spaces`.
Persisted directories live under `@persist/dirs`; persisted files live under
`@persist/.immutability/files`.

The reset path records the active immutability state version in
`@snapshots/.immutability/version`, mounted at
`/.snapshots/.immutability/version` after boot. The current version is
`2026.08.22`; any future version-specific transition belongs in
`config/system/storage/immutability/default.nix`.

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

`config/pkgs/de/kde/default.nix` enables the Plasma 6 Wayland desktop, SDDM, dconf,
graphics, cursor packages, and selected KDE integrations. The display manager
starts `settings.desktop.defaultSession`.

`config/pkgs/de/kde/` owns the Plasma system module, Home Manager configuration,
KDE option definitions, and persistence paths. Shared desktop options live in
`config/pkgs/de/options.nix`; their existing `settings.desktop.*` names are unchanged.

Host-specific desktop state lives in the host modules. FRACTAL-NORTH and Steam Deck
use `default.nix` entries that explicitly import `system/` and `pkgs/`. Package
selections and hardware-specific overrides live in `pkgs/default.nix`, while shared
application implementations live under `config/pkgs/`. Supporting assets stay beside
their modules. The smaller Mac and QEMU hosts use one `default.nix` each.

| File | Role |
|---|---|
| `config/hosts/x86_64/FRACTAL-NORTH/system/gpu.nix` | NVIDIA/AMD graphics setup, device symlinks, KWin DRM devices |
| `config/hosts/x86_64/FRACTAL-NORTH/system/audio.nix` | Low-latency PipeWire and device-specific audio rules |
| `config/hosts/x86_64/FRACTAL-NORTH/system/input.nix` | Controller and mouse receiver support |

### Networking And Security

`config/system/nixos/networking.nix` owns NetworkManager, firewall rules, mDNS,
OpenSSH, SSH agent forwarding, and optional primary-interface enforcement.
Firewall LAN allow rules are derived from shared service flags and
`settings.networking.ports.{tcp,udp}`.

`config/nixos.nix` explicitly composes system and package modules.
`config/system/nixos/users.nix` imports Home Manager and defines immutable users:
the interactive admin account and
an isolated agent account with its own Home Manager profile and tool set. User
mutability is disabled, so password changes must update the hashed password file
and rebuild.

`settings.users.admin.hashedPasswordFile` defaults to
`/var/lib/nixos/credentials/password-hash`. Its directory is root-owned `0700`,
the file is root-owned `0600`, and every parent must be root-owned and not
writable by other users. Installation copies it explicitly; immutable-root
systems mount the persistent `/var` subvolume before activation. Password updates use an
exclusive lock and durable atomic replacement.
The configuration rejects a credential location that would disappear during an
immutable-root reset or be mounted too late for activation. Restoring an older
root snapshot keeps the current password from persistent storage.
The containing mounts must be required (`nofail` and `noauto` are rejected),
and the hash itself cannot be a bind mount because updates replace it atomically.

Use `nixosctl change-password` for persistent password changes. A direct `passwd`
changes the running account only; with immutable users, activation restores the
authoritative hash on rebuild or boot.
Disk, running-account and persistent-password changes are separate operations.
If saving the persistent hash fails after the account changes, the CLI explains
that reboot may restore the previous account password while keeping a completed
disk-password change. Rerun `change-password` with the intended new password to
finish; it skips components that already accept that password. If only the final
rebuild fails, the new hash is already saved; run `nixosctl update` before rebooting
so the booted generation uses that authority rather than a legacy repository hash.

The installer uses `/run/nixosctl-install/passphrase` for the temporary LUKS
passphrase. It creates the directory as root-only `0700` and the file as `0600`
before writing, rejects unsafe existing files and ancestors, and removes a file
it created after disk preparation, including ordinary failure paths. A hard kill
can leave the protected file until reboot; `/run` is temporary. A securely
pre-provided file is preserved for its caller. Custom
`settings.disk.encryption.plainTextPasswordFile` paths must meet the same ownership
and permission rules; shared writable directories such as `/tmp` are rejected.
Supplied files must contain the exact passphrase bytes, without a trailing
newline, CR, or NUL, and at most 511 bytes (the account-hash limit, also within
the unlock PAM limit). A supplied file takes precedence over a hash left by an
earlier installer attempt so disk and account passwords agree.
The limit applies to new passwords; a longer existing LUKS passphrase can still
be supplied to `change-password` when rotating it to a supported new password.

On the first upgrade, the helper preserves the effective password from
`/etc/shadow` if the new file does not yet exist. It does not trust the legacy
file in the editable repository. Missing or unsafe credentials abort activation
before account updates. Old generations still use their original password-file
configuration; retain their legacy file until those generations are retired.

Administration uses ordinary sudo authentication, including the configured
1Password SSH agent. The separate agent account has no administrative access.
Local admin shells select 1Password's socket through `SSH_AUTH_SOCK`, which
sudo's PAM module needs independently of SSH's `IdentityAgent` setting.
SSH sessions retain their forwarded agent. For a terminal opened before this
configuration was activated, run `export SSH_AUTH_SOCK="$HOME/.1password/agent.sock"`.

Every host trusts one SSH key, the `SSH Key` item in 1Password's `Private`
vault, declared once in `settings.users.admin.authorizedKeys`. The Mac declares
the same key for Remote Login (`settings.macos.remoteLogin.enable`, public keys
only), points `ssh` at 1Password's agent, and links launchd's `SSH_AUTH_SOCK` to
that agent so programs that ignore `ssh_config` use it too. The `ssh-trust`
check fails when a host trusts any other key or accepts passwords.

Sunshine supports administration with the SSH keys held in your local 1Password
agent. Enable `settings.apps.gaming.sunshine.sshLogin.enable`; its public keys default
to `settings.users.admin.authorizedKeys`. Existing Sunshine passwords keep
working, and a new instance needs no shared password. Sign in by running the
browser's displayed helper command and approving the agent signature. See
[Sunshine administration](config/pkgs/gaming/sunshine/README.md) for public-key
files, certificate trust, exact commands, session limits and VM checks.

For password login, Sunshine administration credentials are declared with
`settings.apps.gaming.sunshine.credentials.usernameReference` and `passwordReference`.
Defaults reference the `username` and `password` fields of `Sunshine HOSTNAME` in
1Password's `Private` vault. Enable 1Password's Developer → Integrate with
1Password CLI setting, then run as the configured desktop user:

```sh
nixosctl sunshine sync
nixosctl sunshine status
systemctl --user restart sunshine
```

Secrets are retrieved only by the explicit sync command. The local salted
verifier is stored atomically in the user's private
`.config/sunshine/credentials.json`, allowing offline startup. Existing valid
credentials are imported from `sunshine_state.json` on first service start;
Moonlight pairings, host identity, and certificates stay in their original files.
Updating credentials does not rewrite that pairing state. Without SSH keys,
missing or invalid credentials prevent Sunshine from opening its first-run
administration interface, including when launched directly. Configured SSH keys
protect first-run account creation by requiring SSH authentication.

For remote provisioning, `nixosctl sunshine provision` accepts a JSON object with
`username` and `password` through stdin. Feed it through a private pipe from the
credential provider; do not place secret values in command arguments, shell
history, files, or chat. Run it as the remote desktop user, then restart that
user's Sunshine service. Vault retrieval failures leave existing credentials
intact.

`settings.networking.identityAgent` retains the local 1Password agent.
`settings.networking.forwardAgentHosts` explicitly lists destinations allowed to
use that agent remotely; other destinations default to no forwarding. A trusted
remote root can use a forwarded agent while connected, so keep this list narrow.
Inside an SSH session with a forwarded agent, outgoing SSH/Git uses that incoming
agent rather than requiring a second unlocked 1Password app on the remote host.

### Secure Boot

`settings.boot.method = "Secure-Boot"` disables systemd-boot and hands the ESP
to Lanzaboote, which signs the unified kernel images with the PKI bundle at
`settings.boot.pkiBundle` (`/var/lib/sbctl`). `boot.lanzaboote.settings.editor`
is false, so the loader offers no interactive command line to edit.

Enrollment and teardown go through `nixosctl`, never through raw `sbctl`:

```bash
sudo nixosctl secure-boot status           # current enforcement, ownership, signatures
sudo nixosctl secure-boot prepare          # reuse/create keys and install signed images
sudo nixosctl secure-boot enable           # enroll missing own keys, PK last
sudo nixosctl secure-boot disable          # authenticated PK removal; keep signed images
```

`prepare` validates and reuses a complete local PK/KEK/db bundle, creating keys
only when none exist. It selects the signed target, rebuilds and switches the
system, verifies its boot images, and prints public certificate fingerprints and
the signed recovery artifact location. It does not enroll firmware keys. Before
preparation or any key change, each firmware database must be absent or contain
only the matching local certificate. Foreign or mixed trust is refused before
the rebuild; migrating a machine with factory keys requires a separate,
deliberate firmware procedure.

`enable` prepares and verifies images before enrolling missing databases, with
PK last. It preserves every existing owned database byte for byte. If all our
keys are already enrolled, it verifies the existing configuration without a
rebuild or firmware write. An enrolled PK with missing KEK/db is reported as
incomplete rather than silently repaired. Enrollment requires Setup Mode when
PK is absent. `enable` and `disable` accept `--no-reboot` to suppress their reboot
prompt; use `status` after a restart to confirm actual enforcement.

Verification covers the fallback loader, `systemd-boot<arch>.efi`, the default
boot entry resolved from `bootctl --json=short list`, and every
`EFI/Linux/*.efi`. When `settings.disk.recovery.enable` is set it additionally
checks the recovery UKI on both the ESP and the recovery partition, plus the
recovery partition's fallback loader, with `sbverify --cert` against our db
certificate.

Only our own keys are supported: `--microsoft` is rejected. Enrollment keeps
sbctl's option-ROM checks and does not pass its dangerous bypass flag. A refusal
requires investigating the actual firmware and option-ROM requirements, not
silently broadening trust. Independently back up the keys, verify restoration,
and boot signed external recovery before relying on this workflow on hardware.

`disable` authenticates removal of PK with its private key. It retains KEK/db,
the local signing bundle, the signed target and all signed boot/recovery images.
Repeated removal is harmless. It refuses stale enrollment assets, an active
enrollment service, unsafe current/booted or retained generations, orphaned
UKIs, and ambiguous legacy loader entries. Boot a safe signed generation and
prune/rebuild the old menu before retrying. Automatic enrollment is unsupported
in repository configurations; the loader explicitly uses `secure-boot-enroll
off`. This prevents a later boot from undoing deliberate PK removal.

`status` reads firmware state and verifies artifacts without changing keys or
configuration. Current enforcement and pending key changes are reported
separately: removing PK may still require a reboot, and an enrolled PK alone
does not prove firmware enforcement. Switching to `Standard-Boot` remains a
separate operation, guarded against installing unsigned images while firmware
still enforces Secure Boot.

### Firmware Security Posture

`fwupdmgr security` is the health check for the platform side of security.
`FRACTAL-NORTH` sits at **HSI:1**, which is the ceiling for a consumer AM5
board: HSI-2 needs AMD Platform Secure Boot, an OEM fuse ASUS does not burn.
The required BIOS settings, the expected report, and the re-check routine after
every firmware flash live in
[`config/hosts/x86_64/FRACTAL-NORTH/BIOS.md`](config/hosts/x86_64/FRACTAL-NORTH/BIOS.md).

Two facts are easy to forget:

1. ASUS resets AMD CBS settings on every BIOS flash. Secure Boot and TSME both
   dropped silently on 2026-05-18 that way. Re-run the checklist after flashing.
2. The board's ACPI never marks the Thunderbolt 4 root port `ExternalFacingPort`,
   so the kernel cannot flag tunneled devices as untrusted. `cpu.nix` therefore
   leaves the IOMMU in translated mode (no `iommu=pt`); that is the only DMA
   protection Thunderbolt gets on this hardware.

## Apps And Package Overrides

`config/pkgs/default.nix` imports explicit dev, gaming and utility catalogs.
Application modules use the validated hierarchy under `settings.apps`. Groups let lean
profiles turn off whole groups:
`settings.apps.dev.enable`, `settings.apps.gaming.enable`, and
`settings.apps.utils.enable`.

| Path | Behavior |
|---|---|
| `dev/zsh.nix` | Makes zsh the admin login shell when `settings.apps.dev.zsh.enable` is set, leaving compinit to Home Manager |
| `dev/git.nix` | Enables system Git; per-user identities live in Home Manager |
| `dev/vscode.nix` | Installs VSCode, the BTRC extension, and `btrc-lsp` from the flake-locked BTRC package set |
| `gaming/battlenet/default.nix` | Installs a Battle.net Proton wrapper, desktop entry, and capture helper on x86_64 when Steam is enabled |
| `gaming/proton/default.nix` | Installs the patched Proton compatibility tool when `settings.apps.gaming.steam.protonCustom.enable` is set |
| `gaming/rocksmith/default.nix` | Installs Rocksmith support files and Slopsmith when Steam is enabled |
| `gaming/steam/default.nix` | Exports Steam tool names, configures installed Steam apps, and installs `settings.apps.gaming.steam.shortcuts` as non-Steam shortcuts when Steam is enabled |
| `gaming/sunshine/default.nix` | Sunshine service and ports; the CUDA overlay, and the KWin HDR screencast patch when capture is `kwin` |
| `gaming/sunshine/virtual-display.nix` | KWin virtual streaming display and its Desktop/Big Picture apps when `virtualDisplay.enable` is set |
| `gaming/assettocorsa/default.nix` | Assetto Corsa content import, CM presets, control bindings, UI scale, and the CSP wheel menu |
| `gaming/simracing/default.nix` | Enables the `logitech-trueforce` kernel module on x86_64 for wheel force feedback |
| `gaming/semu/default.nix` | Installs `semu` from its flake input, sets `SEMU_TARGET` (and `SEMU_ROOT` when `root` is set), and declares the library as an rclone offline folder (`drive`) |
| `gaming/btrsmith/default.nix` | Installs BTRSmith when an exact flake input provides it |
| `gaming/wine/default.nix` | `wine-prefix-dpi`, which writes `LogPixels` into a prefix registry |
| `utils/1password.nix` | Installs 1Password GUI/CLI and writes SSH agent config |
| `utils/nixosctl/default.nix` | Builds + packages the BTRC `nixosctl` CLI into `environment.systemPackages` |
| `utils/nixosctl/ui/default.nix` | Packages the BTRC tray GUI for `nixosctl` |
| `utils/rclone.nix` | Defines a user rclone mount for `~/Drive`, and an hourly two-way `rclone bisync` for each `settings.apps.utils.rclone.offline` folder |

The custom Proton tree under `config/pkgs/gaming/proton/custom` is a
large patch-carrying workflow for Battle.net/Wayland/HDR behavior. Keep that
area focused and avoid broad formatting churn; its local README has the
development loop.

VFIO has been removed from this repo for now because the path was broken and
explicitly out of scope for the BTRC rewrite.

## BTRC Management Layer

Management runtime source lives alongside its Nix modules under `config/pkgs/utils/nixosctl/`
and `config/system/storage/immutability/`. The compiler, language
server, and VSCode extension are consumed through the flake input `inputs.btrc`.
The dev shell provides `btrcpy` and `btrc-lsp`; local and Nix builds compile
with `--strict-imports`.

That input tracks upstream's default branch; `flake.lock` records the revision
in use and `nix flake update btrc` moves it.

| Path | Purpose |
|---|---|
| `config/pkgs/utils/nixosctl/Main.btrc` | Main CLI (also hosts the VM e2e harness: `nixosctl graph` / `e2e`) |
| `config/pkgs/utils/nixosctl/ui/Main.btrc` | Tray launcher for `nixosctl` |
| `config/pkgs/utils/nixosctl/lib/btrfs` | Snapshot and diff helpers for `nixosctl` |
| `config/pkgs/utils/nixosctl/lib/desktop` | Audio, display, caffeine, and hardware config commands |
| `config/pkgs/utils/nixosctl/lib/hardware` | Secure Boot (`SecureBootManager` + the `SecureBootVerification` parsers) and TPM2 commands |
| `config/system/storage/immutability/src/Immutability.btrc` | BTRC entrypoint for the immutability runtime |
| `config/system/storage/immutability/src/lib` | Immutability runtime library modules |
| `config/pkgs/utils/nixosctl/lib/core` | Shared logging, path, and interactive helpers |
| `config/pkgs/gaming` | Independently packaged Steam, Wine prefix, and Assetto Corsa runtime tools |
| `config/pkgs/utils/nixosctl/lib/simulator` | Declarative VM graph and test runner |

BTRC class modules use PascalCase filenames; executable entry points use `Main.btrc`.
The Makefile scans BTRC sources under `config/` and `tests/`. The VM graph hashes
workspace source files and the runner binary; source-derived states and their
descendants become stale when those inputs change.

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

On installed hosts the CLI is packaged as `nixosctl` (see `config/pkgs/utils/nixosctl/default.nix`)
and is on `PATH`. The native system tray (macOS menu bar / Linux Wayland
StatusNotifierItem) is implemented in the BTRC stdlib and packaged by
`config/pkgs/utils/nixosctl/ui/default.nix`.

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
- `install-nixosctl` builds the packaged CLI, copies its closure into the guest,
  and runs `nixosctl install --format` inside the installer ISO. The live ISO keeps its
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
change: the Makefile targets, `repo-tools publish` and both GitHub workflows read that file.

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

`config/system/nixos/nix.nix` applies them, with `connect-timeout = 5` and `fallback` so a
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
| `secrets/` | Legacy/local secret material; authoritative account hashes live under `/var/lib/nixos/credentials` |
| `config.json` | Local selected host/target for helper tooling |
| `.vm/` | QEMU disks, ISO downloads, state graph data |
| `.direnv/`, `.btrc-cache/`, `build/` | Development caches and build outputs |
| `config/pkgs/gaming/proton/custom/src/` | Working Wine tree for the proton-custom dev loop (`make shell` / `make setup`) |
| `config/pkgs/gaming/battlenet/tests/.build/` | Battle.net test scratch output |
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
4. Keep QEMU architecture assumptions inside the matching `config/hosts/<arch>/QEMU`.
5. Do not add tests that only assert that broken or unimplemented behavior is absent.
6. Add positive tests for expected behavior.
7. Run at least the BTRC build and relevant graph node after touching BTRC or VM test code.
8. Do not reintroduce VFIO until there is a clean design and expected-behavior test path.
9. Do not remove desktop packages or host customizations while making cross-platform changes unless they are concretely unsupported on that platform.

## macOS

The Mac configuration is in [mbp21](config/hosts/aarch64/mbp21/README.md).
Its single `default.nix` contains host package selections, shell overrides,
Home Manager configuration and permission intent. `config/outputs/hosts.nix`
selects Darwin for this host, and `config/darwin.nix` composes its platform modules.
The standalone Home Manager output reuses the Darwin system's user declaration.

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
