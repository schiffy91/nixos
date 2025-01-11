# TO DO
* KDE
    * Plasma: Put applications on bottom bar
    * Konsole: Remove menu bar
    * Mouse: Back/forward mouse buttons
    * KWallet: Figure out how to create a default wallet that matches the hashed password in `/etc/nixois/secrets/hashed_password.txt`
* Applications
    * Google Drive
    * Apple Music
    * Solaar
    * 1Password
        * Autostart 1Password
        * Ability to unlock 1Password via terminal [op signin should work](https://developer.1password.com/docs/cli/sign-in-sso/)
* Code
    * `nixos.py`: `Utils` should probably log to an actual log in lieu of (or in addition to) `stdout` and `stderr`.
* Boot
    * 1Password
        * Create a service to autostart 1Password
        * Investigate ability to unlock 1Password via terminal instead of GUI. Se
        * Investigate if there's a way to automatically reduce prompts for password or a way to automate the extraction of the SSH key after login.
* Gaming
    * Connect 4TB NVME over TB4
    * Configure QEMU for VFIO, IOMMU, and Looking Glass
    * VFIO, IOMMU, and Looking glass.
        * Some examples from the community
            * https://olai.dev/blog/nvidia-vm-passthrough/
            * https://astrid.tech/2022/09/22/0/nixos-gpu-vfio/
            * https://gist.github.com/CRTified/43b7ce84cd238673f7f24652c85980b3
            * https://github.com/j-brn/nixos-vfio
