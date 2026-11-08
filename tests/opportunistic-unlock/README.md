# Opportunistic LUKS unlock

`settings.disk.encryption.opportunisticUnlock.enable = true` tries the passphrase
that successfully unlocked root against eligible encrypted volumes already
attached at boot. The option defaults to disabled; FRACTAL-NORTH and
STEAM-DECK-OLED enable it, while QEMU hosts leave it disabled. No UUID registry,
password file, extra prompt or persistent enrollment is required. The worker never formats a
device and never mounts a filesystem.

The initrd-only systemd patch hands a verified passphrase to userspace through a
root-owned kernel key with owner-only permissions and a 45-second expiry.
Rejected candidates, keyfile/token unlocks, already-active mappings and TPM-only
boots cannot create this handoff. Measured passphrase activation follows the
same success check. The worker consumes and revokes the handoff before reading
optional volume headers; the ordinary systemd password cache is not treated as
proof that a candidate unlocked root.

Discovery takes one fixed inventory of local disks and partitions. Native SD,
USB, SATA/virtio and NVMe storage use the same rules. The configured root backing
and overlapping partitions are excluded, as are devices already backing an
active mapping under any name. Device numbers and kernel disk sequence identity
are checked again before activation, so removal and replacement invalidate an
inventory entry. Mapping names use device identity rather than LUKS UUID, allowing
separate volumes with cloned UUIDs without collisions. Later device arrival does
not start another scan.

Only supported software LUKS1/LUKS2 headers are considered. OPAL hardware
encryption, reencryption states, integrity modes, null/unsupported ciphers,
unknown requirements and excessive KDF parameters are skipped. Ordinary AES-XTS
and legacy AES-CBC-ESSIV with supported PBKDF2 or Argon2 parameters remain
eligible. The policy permits at most 1 GiB of Argon memory and eight threads;
actual device attempts also have time and process memory limits.

Two per-device workers process the fixed inventory at once, each with an
eight-second deadline; the whole run has a 20-second budget.
A slow or failing device cannot consume all worker slots indefinitely. Password
memory is locked and erased, core dumps and swap are disabled, and the service
has a 25-second runtime limit. `Type=exec` acknowledges process startup, so boot
does not wait for optional-volume work. A task stuck in uninterruptible kernel
I/O may outlive its kill signal; the coordinator never waits indefinitely for it.

The worker is BTRC, split into device inventory, header policy, secret memory and
scheduling modules. Passphrases and volume keys stay in a raw locked mapping;
they never enter a managed string or collection. Each child locks its inherited
private mapping before using it, and clears the password before activating the
verified volume key. The only native adapter is a pair of ioctl constant aliases;
the existing handoff hook remains in upstream systemd's C source.

Successful mappings remain unlocked and unmounted. Mounting belongs to the
normal desktop or declared mount configuration. Wrong passwords, plain devices,
unsupported headers, missing/expired keys and I/O errors quietly skip. Manual
unlocking remains available independently of this feature.

## Regression gates

```sh
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run opportunistic-unlock'
```

The graph runs `opportunistic-unlock-policy` before the VM. The small gate tests
production BTRC policy for OPAL, reencryption, unknown requirements, malformed
metadata and resource limits. Memory tests verify locking, fork isolation,
password/key wipe boundaries and disk generation overlap rules. Generated-code
checks reject managed-string/ARC/thread runtime in the worker and verify secure
wipes survive optimization. The gate also uses real sparse
LUKS1/LUKS2 files and libcryptsetup to verify supported, unsupported and malformed
headers, including a header with all keyslots erased, without a VM or physical
block device.

The boot gate uses a disposable encrypted-root VM with a software TPM, native
SD controller, USB disk, virtio disk and partitioned NVMe disk. It exercises real
console passwords and switch-root handoff; owner/user-namespace access denial;
verified-candidate provenance; expiry, consumption and revocation; measured,
keyfile and TPM activation; software LUKS1/LUKS2; wrong passwords; root and
arbitrary-name already-open exclusions; duplicate UUIDs; replacement; read
errors; a stopped first candidate followed by a healthy device; inherited
ignored SIGCHLD without stale worker slots; no automatic
mounting; no later-hotplug retry; automatic service startup; and TPM-only boot.
All images and credentials are fixtures. No physical host or storage is used.
