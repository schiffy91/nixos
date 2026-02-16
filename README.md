# NixOS Configuration

Declarative, reproducible NixOS system management with BTRFS immutability, LUKS encryption, Secure Boot, and automated testing.

## Architecture

```
/etc/nixos/
  flake.nix              # System flake (nixpkgs, home-manager, disko, lanzaboote)
  config.json            # Local machine state (host path, boot target)
  modules/
    settings.nix         # Global option declarations with defaults
    hosts/               # Per-machine configurations
      x86_64/
        FRACTAL-NORTH.nix  # Desktop: AMD + RTX 4090, VFIO, immutability
        VM-TEST.nix        # QEMU test VM
      aarch64/
        Parallels.nix      # macOS ARM VM
        UTM.nix            # macOS ARM VM
    system/              # System modules
      admin.nix          # User accounts, home-manager
      boot.nix           # Bootloader, Secure Boot (lanzaboote)
      desktop.nix        # Desktop environment (Plasma/GNOME/Hyprland)
      disk.nix           # Disko declarative partitioning
      immutability.nix   # BTRFS snapshot-based factory reset on boot
      locale.nix         # Timezone, i18n
      networking.nix     # Firewall, VPN, SSH
      packages.nix       # System packages
      sound.nix          # PipeWire audio
    apps/                # Application modules
      1password.nix      # 1Password + SSH agent
      bash.nix           # Shell configuration
      git.nix            # Git config
      plasma.nix         # Plasma/KDE settings (via plasma-manager)
      vscode.nix         # VS Code extensions and settings
      virt.nix           # Virtualization (libvirtd, QEMU)
  bin/                   # Executable CLI tools (nix-shell shebangs)
  lib/                   # Python library modules
  tests/                 # Unit, integration, and functional tests
  .vm/                   # QEMU VM test environment (gitignored)
  secrets/               # Encrypted secrets (gitignored)
```

## Boot Targets

Every host generates three NixOS configurations:

| Target | Purpose |
|---|---|
| `Standard-Boot` | Normal boot with all system modules |
| `Secure-Boot` | Boot with lanzaboote Secure Boot |
| `Disk-Operation` | Minimal config for disko partitioning |

Usage: `nixos-rebuild switch --flake /etc/nixos#HOSTNAME-TARGET`

## Immutability

When `settings.disk.immutability.enable = true`, the system factory-resets designated BTRFS subvolumes (`@root`, `@home`) on every boot via an initrd systemd service.

**How it works:**
1. At build time, an rsync filter file is precomputed from `settings.disk.immutability.persist.paths`
2. At boot (initrd), the service mounts the BTRFS root and snapshots subvolume
3. Current subvolume state is saved to `PREVIOUS` snapshot
4. A fresh copy of the `CLEAN` snapshot is created as `CURRENT`
5. Persistent paths are rsynced from `PREVIOUS` into `CURRENT`
6. A `.boot-ready` sentinel is written for crash recovery
7. `CURRENT` replaces the live subvolume

Persistent paths include `/etc/nixos`, `/etc/ssh`, `/var/lib/nixos`, user config directories, and application data.

## Scripts

### Library (`lib/`)

| Module | Description |
|---|---|
| `shell.py` | Command execution, file I/O, JSON, chroot support, `@chrootable` decorator |
| `config.py` | NixOS configuration management, nix eval caching, secrets, `nixos-rebuild` |
| `utils.py` | Logging, argument parsing, error handling, `@chrootable` decorator support |
| `interactive.py` | User prompts: confirm, host selection, password, reboot |
| `snapshot.py` | BTRFS snapshot management for immutability setup |
| `vm.py` | QEMU VM test harness with serial console and snapshot support |

The `@chrootable` decorator adds a `chroot(sh)` context manager to classes, allowing them to transparently operate inside a mounted NixOS installation (used during `install.py`).

### CLI Tools (`bin/`)

All CLI scripts use nix-shell shebangs and are self-contained:

| Script | Description |
|---|---|
| `install.py` | Full NixOS installation: disko partitioning, nixos-install, permissions, snapshots |
| `update.py` | System update: `nixos-rebuild switch` with optional `--upgrade`, `--clean` |
| `diff.py` | Show what changed since last boot (files that would be wiped by immutability) |
| `eval.py` | Evaluate NixOS configuration attributes via `nix eval` |
| `secure_boot.py` | Enroll Secure Boot keys and switch to lanzaboote target |
| `tpm2.py` | Enroll TPM2 LUKS auto-unlock |
| `change_password.py` | Change user password and re-enroll TPM2 |
| `gpu_vfio.py` | NVIDIA GPU VFIO passthrough: attach/detach/status |

### Usage

```bash
# Install NixOS from live USB
sudo bin/install.py

# Update system
sudo bin/update.py
sudo bin/update.py --upgrade  # Also update flake inputs
sudo bin/update.py --clean    # Garbage collect first

# Evaluate a config attribute
bin/eval.py config.settings.disk.device

# GPU passthrough
sudo bin/gpu_vfio.py status
sudo bin/gpu_vfio.py detach   # Bind to vfio-pci for VM
sudo bin/gpu_vfio.py attach   # Return to nvidia driver
```

## Testing

### Unit and Integration Tests

```bash
python3 -m pytest                              # All fast tests (default)
python3 -m pytest tests/unit_tests/ -v         # Unit tests only
python3 -m pytest tests/integration_tests/ -v  # Integration tests only
```

### Functional Tests (QEMU VM)

The VM test harness (`lib/vm.py`) boots a QEMU VM and runs end-to-end tests with QEMU snapshots as checkpoints for reproducibility.

```bash
# Run all functional tests
python3 -m pytest tests/functional_tests/ -v -s --tb=short

# Clean and rerun from scratch
VM_CLEAN=1 python3 -m pytest tests/functional_tests/ -v -s --tb=short

# Resume from a specific checkpoint
VM_FROM=booted python3 -m pytest tests/functional_tests/ -v -s --tb=short
```

VM tests validate: installation, boot, immutability modes (reset, snapshot-only, disabled, restore-previous, restore-penultimate), performance, and system updates.

## Hardware: FRACTAL-NORTH

- **CPU**: AMD (with iGPU for display output via Thunderbolt 4)
- **GPU**: NVIDIA RTX 4090 (PCIe, rendering via PRIME Sync)
- **Disk**: NVMe with LUKS encryption, BTRFS, 6 subvolumes
- **Boot**: Secure Boot via lanzaboote, TPM2 auto-unlock
- **Features**: VFIO GPU passthrough, Sunshine streaming, Mullvad VPN, libvirtd/QEMU
