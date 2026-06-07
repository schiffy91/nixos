# x86_64 QEMU Host

This host is intentionally lean and VM-shaped:

- architecture: `x86_64-linux`
- host entry: `nix/hosts/x86_64/QEMU/QEMU.nix`
- primary disk: `/dev/vda`
- firmware: UEFI with persistent variable storage
- serial console: `ttyS0`
- optional shared folder: virtio 9p tag `share`, mounted at `/mnt/shared`

Keep QEMU-specific boot, firmware, and device assumptions inside this folder.
The rest of the system should not need to know whether the guest is running on
x86_64 or aarch64.
