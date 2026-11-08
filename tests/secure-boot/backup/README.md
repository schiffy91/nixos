# Independent Secure Boot key backup

`nixosctl secure-boot keys export|import|verify /absolute/bundle/path` transfers
existing sbctl keys. It never generates keys, enrolls firmware, changes boot
entries, or rebuilds a host. These commands require an existing root session
or explicit `sudo -n`; they never auto-elevate or request authentication.

The archive contains exactly `GUID` and the PK/KEK/db `.key`/`.pem` pairs.
Before exporting or importing, the helper checks each private key mathematically
and compares its public key with the certificate. It preserves the original
seven files byte for byte. `verify` outputs only the public owner GUID and SHA256
certificate fingerprints. Export refuses terminal stdout. Import reads a bounded
uncompressed archive from stdin, rejects missing/duplicate/linked/unexpected
members, and publishes a complete directory with an atomic no-replace rename.

Every path component must be a real root-owned directory without group/other
write access. Source files must be regular root-owned files with one hardlink;
private keys must be mode 0400 or 0600. Imports create directories mode 0700 and
files mode 0600, refuse every existing destination, and never write into the Nix
store. Core dumps are disabled. Invalid or interrupted input is rejected before
creating the destination; failed writes remove the private staging directory.
A SIGKILL or power failure during staging can leave a root-only
`.secure-boot-keys-*` sibling, never an incomplete published destination. Do not
remove an existing bundle to make a retry succeed: verify it first.

The helper checks filesystem authority, not disk encryption or physical device
independence. The operator must verify those properties before choosing storage.

## Deck to Fractal procedure

Keep the Deck on Standard-Boot and firmware Secure Boot disabled during
preparation. The Deck defaults to `settings.boot.autoEnrollKeys = false`;
retain that setting before activating any Secure-Boot configuration.
Do not run enrollment, `nixosctl secure-boot enable`, or a boot-policy change
merely to create or back up keys.

1. Confirm the configured `settings.boot.pkiBundle` on the Deck (currently
   `/var/lib/sbctl`). If a bundle already exists, run `keys verify` and reuse it.
   An incomplete bundle is an error to investigate, never permission to rotate
   or overwrite its surviving keys. If the bundle is truly absent, explicitly
   run `sbctl create-keys` once on the Deck, then verify it. Key generation alone
   does not enroll firmware. The helper does not perform this step.
2. On Fractal North, inspect `findmnt -T /var/lib/nixos` and `lsblk -o
   NAME,TYPE,FSTYPE,MOUNTPOINTS,MODEL,SERIAL`. Confirm `/var` is on its local LUKS
   volume and that its physical disk is separate from the Deck's SSD. The
   proposed destination is
   `/var/lib/nixos/secureboot-backups/STEAM-DECK-OLED`, on that encrypted `/var`.
   A second directory or recovery partition on the Deck itself is not an
   independent backup. The helper creates missing root-owned parents privately.
3. Use an already authorized user SSH control connection with its host key
   verified. On Fractal North, check the existing control socket first, then
   stream directly into the local root importer. Substitute the actual existing
   socket and configured bundle path; these commands do not create a connection
   through an authentication agent:

   ```bash
   deck_socket=/path/to/existing/authorized/control-socket
   deck_host='<user of the existing connection>@192.168.50.52'
   backup=/var/lib/nixos/secureboot-backups/STEAM-DECK-OLED
   ssh -S "$deck_socket" -O check "$deck_host"
   set -o pipefail
   ssh -T -a -S "$deck_socket" -o ControlMaster=no -o BatchMode=yes \
     -o PreferredAuthentications=none -o IdentityAgent=none \
     -o StrictHostKeyChecking=yes "$deck_host" \
     'env -u SSH_AUTH_SOCK sudo -n nixosctl secure-boot keys export /var/lib/sbctl' |
     env -u SSH_AUTH_SOCK sudo -n nixosctl secure-boot keys import "$backup"
   ```

   Do not use `tee`, shell tracing, command substitution, terminal output, or a
   log collector for the private archive. A missing/expired authorized transport
   fails without an agent prompt; an incomplete stream publishes nothing. If
   the destination already exists, compare its public verification result with
   the Deck instead of overwriting it.
4. Compare `keys verify /var/lib/sbctl` on the Deck with `sudo -n nixosctl
   secure-boot keys verify "$backup"` on Fractal. Then prove restoration into a
   new absent directory on Fractal's encrypted `/var`:

   ```bash
   set -o pipefail
   env -u SSH_AUTH_SOCK sudo -n nixosctl secure-boot keys export "$backup" |
     env -u SSH_AUTH_SOCK sudo -n nixosctl secure-boot keys import "$backup.restore-check"
   env -u SSH_AUTH_SOCK sudo -n nixosctl secure-boot keys verify "$backup.restore-check"
   ```

   All three public reports must agree. Keep the independent backup. Restoring
   onto the Deck later uses the same private pipe into an absent staging bundle;
   verify it before deliberately selecting it for signing. The importer never
   merges into or replaces existing production keys.

## Existing external recovery artifact and physical gate

`config/system/nixos/recovery.nix` already exposes the self-contained recovery system
and UKI. No new installer ISO or recovery-image framework is needed. The existing
build entrypoint for an **unsigned** Deck recovery UKI is:

```bash
nix build --no-link --print-out-paths \
  '.#nixosConfigurations.STEAM-DECK-OLED-Secure-Boot.config.system.build.recoverySystem.config.system.build.uki'
```

The result contains `nixos-recovery.efi`. Building this artifact does not activate
a host or enroll keys. Sign a copy outside the Nix store using the Deck's existing db key and
certificate, and verify it with `sbverify --cert ...`. An already installed
Secure-Boot recovery UKI at `/recovery/EFI/recovery/nixos-recovery.efi` can also be
used after verifying its signature against that same certificate. Standard-Boot
recovery's split kernel/initrd is not a substitute for a signed UKI.

For an explicitly identified, already prepared external FAT recovery device,
the signed self-contained UKI can occupy `EFI/BOOT/BOOTX64.EFI` for direct x86_64
firmware fallback boot. This avoids a second loader and auto-enrollment assets.
No private key goes onto the recovery device. Formatting or writing a real USB
is a separate deliberate operation and was not performed here.

The remaining physical gate is to boot that external device on the actual Deck,
verify its keyboard/input, recovery shell and ability to unlock/mount the SSD,
then retain it and the independent key backup before deliberate enrollment or
platform-key removal. Cryptographic verification and OVMF tests do not establish
the Deck's firmware/USB behavior. After eventual enrollment, repeat the external
boot under enforcement. Follow the adjacent `removal` scenario for maintenance
and PK-removal ordering; do not assume firmware updates or a BIOS toggle clear
keys safely.

## VM regression gate

```bash
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run secure-boot-backup'
```

The required node creates two disposable guests with separate encrypted virtual
disks, generates fresh sbctl and SSH fixture keys inside the guests, pins SSH
host keys, and uses the newly packaged CLI by store path while the host profile has an incompatible
helper, over noninteractive SSH with the agent
explicitly disabled. It checks exact-byte round trips, private/certificate
matching, owner GUID/fingerprints, existing-key reuse, refusal to overwrite,
root-only permissions, terminal refusal, symlink/hardlink/unsafe-parent refusal,
truncated/interrupted input, invalid archive entries, corrupt keys, and failed
storage writes. No production keys or host authentication are accessed.
