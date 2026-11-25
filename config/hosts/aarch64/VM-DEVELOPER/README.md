# aarch64 VM-DEVELOPER Host

Full ARM64 Linux development workstation VM for running under SVM on Apple Silicon:

- architecture: `aarch64-linux`
- host entry: `config/hosts/aarch64/VM-DEVELOPER/default.nix`
- inherits the shared KDE Plasma 6 Wayland desktop, Home Manager profile, dev toolchain (`zsh`, `git`, `gh`, `ci-local`, `claude-code`, `codex`, `nixd`, `vscode` with `btrc-lsp` and the BTRC extension), and utilities (`nixosctl`, `drive`, Chromium, 1Password CLI, Mesa/Vulkan tools)
- graphics: VirtIO GPU (`virtio_gpu` in initrd, VirGL OpenGL and Venus Vulkan)
- primary disk: `/dev/vda` (non-encrypted, persistent root and home)
- serial console: `ttyAMA0` alongside `tty0`
- optional shared folder: virtio 9p tag `share`, mounted at `/mnt/shared`

Build a ready-to-boot raw disk image with `nix build .#vm-developer-image` on `aarch64-linux`.
