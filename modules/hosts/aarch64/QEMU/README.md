# aarch64 QEMU Host

This host is intentionally lean and VM-shaped:

- architecture: `aarch64-linux`
- host entry: `modules/hosts/aarch64/QEMU/QEMU.nix`
- primary disk: `/dev/vda`
- firmware: UEFI with persistent variable storage
- serial console: `ttyAMA0`
- optional shared folder: virtio 9p tag `share`, mounted at `/mnt/shared`

Keep QEMU-specific boot, firmware, and device assumptions inside this folder.
The rest of the system should not need to know whether the guest is running on
x86_64 or aarch64.
