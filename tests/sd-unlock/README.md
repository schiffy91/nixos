# Opportunistic SD unlock

`settings.disk.encryption.unlockSD = true` tries the password that actually
unlocked the configured root LUKS mapping on an already inserted native SD card.
The default is disabled. No device UUID list, password file, additional prompt,
or persistent enrollment is created. This feature never formats a card.

The initrd-only systemd patch exports the successful password to a root-owned
kernel key with owner-only permissions and a 45-second expiry. Both ordinary
and TPM-measured passphrase activation must prove success. Token/keyfile unlock,
an already active mapping, and rejected password candidates do not export a
password. The usual systemd password cache is not used as proof of success.

The userspace service consumes and revokes that key before touching an SD card.
It considers only `mmcblk*` disks whose kernel card type is `SD`, including their
partitions, and excludes the configured root device/card. eMMC, USB storage,
NVMe, and unrelated encrypted devices are excluded. The card's CID and device
identity are checked before reading its header and again before activation.
Each LUKS candidate receives one attempt with the successful root password.

The worker has a 20-second total deadline, locked password memory, disabled
core dumps, a 2 GiB memory limit and no swap. Its parent erases its copy after
forking; the child erases its copy before requesting mounts. The service uses
`Type=exec`, so boot does not wait for card I/O or password processing. A stuck
kernel I/O task cannot always be killed immediately, but is sent SIGKILL and
is never waited on indefinitely. The service also has a 25-second runtime bound.

Successful mappings use normal UDisks filesystem mounting for the configured
admin username, retaining filesystem labels and paths such as
`/run/media/<username>/SD`. Mounting asks for no authentication interaction.
Absent/plain cards, wrong passwords, I/O failures, expired handoffs, very slow
KDFs and TPM-only boots quietly skip. Later insertion does not retry. Dolphin's
ordinary manual encrypted-volume unlocking remains available.

## Regression gate

```sh
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run sd-unlock'
```

The required graph node builds the patched systemd with at most four cores and
runs one disposable 4 GiB VM with a genuinely encrypted root, software TPM and
QEMU's native SD controller. Its 256 MiB SD image is sparse and disposable. It
tests the real root password prompt, handoff across switch-root, ordinary-user
and user-namespace denial, key expiry/revocation, rejected-then-successful
password provenance, measured activation, keyfile/TPM exclusions, absent/plain
cards, default LUKS2 Argon2, wrong passwords and manual UDisks fallback, a stalled
child, injected SD read failures, a second boot through automatic service
activation after an incorrect-then-correct console password, a partitioned SD,
and a TPM-only root boot without console input. The non-SD encrypted fixture must stay closed.

The stopped-child case checks that the desktop target remains active and that
the child is killed within the overall deadline; a VM-wide block-device throttle
would stall QEMU itself and would not meaningfully test this contract. The I/O
failure case instead injects read errors into only the SD backend. No physical
host or card is accessed by these tests.
