# TO DO
* System
    * Move more of `modules/system` to lib.mkDefault
* Utils
    * Log to an actual log instead of stdout
* Admin
    * Change auto login to default off; flip it on in the VMs, which we can presume are authenticated.
* Boot
    * Secure Boot
        * `FRACTAL-NORTH`: Set a password on EUFI.
        * `VM-QEMU`: In theory, this should work in QEMU too.
    * SSH
        * Figure out why SSH to decrypt LUKS is broken on `VM-QEMU` and `FRACTAL-NORTH`
    * TPM2
        * Convert `bin/tpm2` from `bash` to `python3`.
        * Call `tpm2` as bunch of sh functions chroot'ed after install, using --unlock-key-file=PATH to tmp/plain_text_password.txt to auomate the decryption.
        * `VM-QEMU`: In theory, this should work in QEMU too.
    * 1Password
        * Create a service to autostart 1Password
        * Investigate ability to unlock 1Password via terminal instead of GUI. Se
        * Investigate if there's a way to automatically reduce prompts for password or a way to automate the extraction of the SSH key after login.
* Installer
    * Update `boot.nix` to use `lib.mkMerge` in-line for better readability like `desktop.nix` or `boot.nix`
    * Validate installer didn't break after the most-recent refactor
* VMs
    * Move `VM-VMware`, `VM-Parallels`, and `VM-Apple-Hypervisor` to gists and out of the repository since they're unused.
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