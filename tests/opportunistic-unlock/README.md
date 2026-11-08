# Opportunistic LUKS unlock

`settings.disk.encryption.opportunisticUnlock.enable = true` tries the password
that successfully unlocked root against eligible local LUKS volumes already
attached at boot. FRACTAL-NORTH and STEAM-DECK-OLED enable it. The worker does one
scan, unlocks matching volumes and exits. It never mounts filesystems or keeps a
password for later hotplug.

Software LUKS verification runs on the host: libcryptsetup reads the header and
keyslots, derives the key locally and verifies it. The storage device does not
receive the password. Hardware OPAL, reencryption and integrity activation are
excluded. Libcryptsetup validates LUKS formats, algorithms and keyslots; this
feature does not impose its own cipher or KDF whitelist.

The initrd hook exports only a successful interactive root passphrase into a
root-owned kernel key with owner-only permissions and a 45-second expiry.
Wrong candidates, keyfiles, tokens, already-active mappings and TPM-only boots
cannot create that handoff. The worker consumes and revokes it before optional
volume work. Passwords and derived keys have separate locked buffers with
explicit erasure, including exception paths; neither enters a managed string or
collection. Each child relocks its inherited password buffer before using it.

Discovery covers local SD, USB, SATA/virtio and PCIe NVMe disks and partitions.
It excludes root storage and overlapping mounted or already-open volumes.
Kernel disk sequence numbers detect unplug/replacement; mapping names use device
identity, so cloned LUKS UUIDs do not collide. Later attachments are outside the
snapshot.

Two attempts run at once so one stalled device does not hold up every other
volume. Each has eight seconds and a 1.5 GiB address-space limit; the session has
20 seconds, backed by the service's 25-second limit and 3 GiB memory ceiling.
Core dumps and swapping are disabled. Excessive KDF work fails or times out
within these limits. `Type=exec` lets normal boot continue immediately. Kernel
I/O stuck in an uninterruptible state can outlive a kill signal; the coordinator
never waits indefinitely for it.

The BTRC implementation uses `BlockVolume`/`VolumeInventory`, `BootPassphrase`/
`LockedSecret`, `LuksVolume`, and `UnlockSession`/`UnlockAttempt`. Native calls
are confined to kernel key/memory/process/device operations and libcryptsetup.
Ordinary metadata uses the stdlib. The coordinator checks that it is single
threaded before forking; children explicitly clean secrets and use `_exit`.
The small systemd success hook remains in upstream systemd's C source.

## Regression gates

```sh
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run opportunistic-unlock'
```

The small predecessor checks handoff ownership, device identity/ranges, locked
memory, exception erasure, child relocking and managed metadata after fork. It
also verifies actual keys from sparse LUKS1/LUKS2 PBKDF2 and Argon2 files,
including an alternate cipher, plus malformed and erased-keyslot failures.

The boot gate uses encrypted-root QEMU with a TPM, SD controller, USB, virtio and
partitioned NVMe disks. It covers successful-password provenance, permissions,
expiry/revocation, wrong passwords, existing mappings, cloned UUIDs, replacement,
read errors, deadlines/fairness, ignored SIGCHLD, no mounting/hotplug retry,
automatic service startup and TPM-only boot. All storage and credentials are
fixtures; no physical host or device is used.
