# TO DO
* Utils
    * Log to a log instead of stdout
* Boot
    * Secure Boot
        * `FRACTAL-NORTH`: 
            * Remove all closed source drivers. 
            * Debug why it's not working.
            * Set a password on EUFI.
        * `VM-QEMU`: In theory, Secure Boot should work there too since everything is open source.
    * SSH
        * Figure out why SSH to decrypt LUKS is broken on `VM-QEMU` and `FRACTAL-NORTH`
    * TPM2
        * Convert `bin/tpm2` from `bash` to `python3`.
        * Get it working on `FRACTAL-NORTH` so that keys are stored there instead of `/etc/secureboot` with `sbctl`
            ```
            λ » sudo sbctl reset
            ✓ Removed Platform Key!
            Use `sbctl enroll-keys` to enroll the Platform Key again.

            λ » sudo sbctl rotate-keys --pk-keytype tpm --kek-keytype kek --db-keytype file
            Backed up keys to /var/tmp/sbctl/sbctl_backup_keys_1722423218
            Creating secure boot keys...✓
            Secure boot keys created!
            ✓ Enrolled new keys into UEFI!
            ✓ Signed /usr/lib/systemd/boot/efi/systemd-bootx64.efi.signed
            ✓ Signed /efi/EFI/BOOT/BOOTX64.EFI
            ✓ Signed /efi/EFI/Linux/arch-linux.efi
            ✓ Signed /usr/lib/fwupd/efi/fwupdx64.efi.signed
            ```
        * `VM-QEMU`: In theory, TPM2 should work there too, but it probably is off the main path for Linux VMs (vs Windows 11 VMs).
* Installer
    * Validate installer didn't break after the most-recent refactor
* VMs
    * Move `VM-VMware`, `VM-Parallels`, and `VM-Apple-Hypervisor` to gists and remove from my repository.
* Desktop Environment
    * PAM for Secrets in KDEWallet
    * Plasma Manager
        * DPI
        * Panels
            * Auto hide top
            * Sort applications on bottom
        * Icons
        * Wallpaper
        * Dolphin
        * Console
    * Applications
        * Google Drive
        * Chrome + Login
        * VSCode + Login
        * Apple Music
        * Solaar
* Virtualization
    * Install 4TB NVME with Windows
    * Auto configure virt-manager to set it up with some config files that are checked in.
    * VFIO, IOMMU, and Looking glass.
    * Ensure BIOS has that at the lowest priority bootloader.