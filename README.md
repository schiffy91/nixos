# NixOS Configuration

Personal NixOS configuration with secure boot, disk encryption, and TPM2 support.

## Features

- Declarative disk formatting with [disko](https://github.com/nix-community/disko)
- Full disk encryption with LUKS
- Secure boot support via [Lanzaboote](https://github.com/nix-community/lanzaboote)
- Interactive password login
- Single password entry for disk decryption and login
- Btrfs with compression and subvolumes
- Minimal KDE Plasma 6 desktop environment
- Support for both x86_64 and ARM64 (Apple Silicon via Parallels)

## Installation

1. Boot from [NixOS installation media](https://nixos.org/download/#nixos-iso)
2. Clone this repository:
   ```sh
   sudo rm -rf /etc/nixos && sudo git clone https://github.com/schiffy91/nixos.git /etc/nixos
   ```
3. Run the installer:
   ```sh
   cd /etc/nixos && sudo ./nixos-install
   ```
   The installer will:
   - Guide you through host selection
   - Set up disk encryption and root user passwords
   - Handle disk formatting and mounting
   - Install NixOS with your configuration

## Post-Install Steps

1. Enable secure boot (optional):
   ```sh
   sudo nixos-enable-secure-boot
   ```

### Parallels Desktop 20 Secure Boot Setup

1. Boot into Firmware Interface
2. Change Secure Boot Mode to `Custom Mode`:
   ```
   Device Manager → Secure Boot Configuration → Secure Boot Mode
   ```
3. Save with F10
4. Edit VM config file:
   - Show Package Contents → Open with TextEdit
   - Change `<EfiSecureBoot>0</EfiSecureBoot>` to `<EfiSecureBoot>2</EfiSecureBoot>`

## System Management

- Update system: `sudo nixos-update`
- Disable secure boot: `sudo nixos-disable-secure-boot`

## Script Usage

### Installation Script (`nixos-install`)

The installation script can be run in two modes:
```sh
# Normal installation
sudo ./nixos-install

# Debug mode - opens VSCode for configuration editing
sudo ./nixos-install --debug
```

The script will:
1. Offer to run garbage collection
2. Install required Python version
3. Execute the installation process

### Secure Boot Management (`secure-boot`)

```sh
# Enable secure boot
sudo ./scripts/secure-boot enable

# Disable secure boot
sudo ./scripts/secure-boot disable
```

When enabling secure boot, the script will:
1. Create and set up required directories
2. Generate secure boot keys
3. Enroll keys with Microsoft compatibility
4. Update system configuration
5. Verify EFI stub creation

### System Updates (`update`)

```sh
sudo ./scripts/update
```

The update script:
1. Verifies root privileges
2. Checks/rebuilds secrets if missing
3. Resets configuration if needed
4. Rebuilds and updates NixOS

## Creating a New Host

To add a new host configuration:

1. Create a new directory under `hosts/`:
   ```
   mkdir -p hosts/your-hostname
   ```

2. Create the following files:
   ```
   hosts/your-hostname/
   ├── default.nix     # Main host configuration
   ├── hardware.nix    # Hardware-specific settings
   └── disks.nix       # Disk partitioning scheme
   ```

3. Add your host to `flake.nix`:
   ```nix
   Configurations = {
     // ...existing hosts...
     your-hostname = lib.nixosSystem {
       system = "x86_64-linux"; # or "aarch64-linux"
       modules = [
         ./hosts/your-hostname
         # Add other required modules
       ];
     };
   };
   ```

4. Minimum required configuration in `default.nix`:
   ```nix
   { config, pkgs, ... }: {
     imports = [
       ./hardware.nix
       ../../modules/base
       # Add other required modules
     ];

     networking.hostName = "your-hostname";
     # Add host-specific configuration
   }
   ```

## Host Configurations

- **MBP-M1-VM**: Apple Silicon virtual machine via Parallels
- **FRACTAL-NORTH**: AMD/NVIDIA hybrid graphics workstation

## Project Structure
```
.
├── scripts/            # System management scripts
├──── nixos-install     # Run from live ISO to install NixOS
├──── secure-boot       # Run from host after installation
├──── tpm2              # Broken :)
├──── update            # Run from host to update your build with changes
├── hosts/              # Host-specific configs
├── modules/            # Common configuration modules
├── configuration.nix   # Base configuration
├── flake.nix           # Flake definition
└── nixos-install       # Installation script
```

## Development Status

Current focus:
- Stabilizing Lanzaboote implementation
- Updating to newer kernel for better Parallels support
- Improving installation script reliability
- Adding home-manager support
- Fixing sleep and hibernate functionality
- Updating system management scripts for secure boot