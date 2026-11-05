# Steam Deck OLED

`STEAM-DECK-OLED.nix` enables Jovian's hardware support and Gaming Mode, a
touchscreen LUKS prompt (unl0kr), hibernation on every sleep, Secure Boot key
re-enrollment and Sunshine with KMS capture. Its `let` block holds the device
data: disk, SD card label and LAN.

Decky Loader runs from Jovian; install plugins from its store. The non-Steam
shortcuts in `settings.apps.steam.shortcuts` are written to every Steam profile
at boot, before Steam starts. Other shortcuts in Steam are left alone.

The disk layout is the shared one: LUKS over btrfs, immutability, the recovery
partition and a swapfile large enough to hibernate. The disk is unlocked only
by the passphrase. Do not enroll a TPM2 keyslot on a Deck.

## Install

Build the installer on another x86_64 machine and write it to a USB drive. The
image carries this repository, the admin's keys, `nixosctl`, this host's whole
system closure, disko's own format and mount scripts and every flake input
source, so the Deck needs no network and builds nothing:

```sh
make iso HOST=STEAM-DECK-OLED
make iso-write DEVICE=/dev/sdX HOST=STEAM-DECK-OLED   # ~10 GB, so a 16 GB stick
```

Hold Volume Down and press Power for the boot menu and pick the USB drive. The
ISO logs in as root and places this repository at a writable `/etc/nixos`; no
hub, no Ethernet and no `git clone`.

1. Run `nixosctl install`, choose `STEAM-DECK-OLED`, and set the password. It
   becomes both the LUKS passphrase and the account password; `nixosctl
   change-password --full-disk-encryption-only` later changes only the
   passphrase. `--format --yes` skips the prompts.

   The log must print `installing the system baked into this installer:`. Without
   that line the stick carries nothing for this host, so the install falls back to
   evaluating the flake and building the Jovian kernel and four Mesa variants,
   which needs the network and more than the Deck's 16 GB. `--force-upgrade`
   deliberately takes that fallback after updating the flake lock.
2. Reboot and type the passphrase on the touchscreen. unl0kr has no rotation, so
   its keyboard follows the panel's native portrait orientation.
3. The firmware ships in Setup Mode. Run `sudo nixosctl secure-boot enable`.

   Our keys only — no `--microsoft`, the same rule as every other host. The
   open question this leaves is the Deck's own firmware path:
   `jupiter-biosupdate` and the `jovian.devices.steamdeck.autoUpdate`
   BIOS/controller updates are Microsoft-signed and run with Secure Boot
   enforcing, so verify a BIOS update still applies before trusting it. If one
   is refused, disable Secure Boot in firmware for that update rather than
   enrolling Microsoft's keys. `settings.boot.autoEnrollKeys` re-enrolls our
   keys after a flash clears them.

## Sleep and firmware

Steam and Plasma ask logind to suspend, and `settings.sleep.hibernateOnSuspend`
turns that into hibernation, so every wake goes through the passphrase. Valve
disabled hibernation on the Deck in the SteamOS 3.9.4 beta (October 2026),
citing driver bugs. If resume misbehaves, set `settings.sleep.hibernateOnSuspend
= false;` in the host file to go back to suspend.

Resume needs no kernel command line. The image lands in the btrfs swapfile, and
`systemd-hibernate` records its device and offset in the `HibernateLocation`
EFI variable, which `systemd-hibernate-resume-generator` reads in the initrd
and clears after a successful resume. That is why `boot.resumeDevice` is empty
everywhere in this repo, and app-settings asserts it stays empty.

**Do not "fix" resume by adding `resume=`.** For a swapfile it needs a matching
`resume_offset=`, which is the file's physical block offset and only knowable
after the filesystem exists. systemd refuses `resume_offset=` without `resume=`,
and a bare `resume=` points at the start of the filesystem rather than the swap
header, so a half-wired pair is worse than none. The swapfile is sized in
`STEAM-DECK-OLED.nix` (`swapSize = "20G"` for 16 GB of RAM) because the image
has to fit, and the initrd reset is ordered after
`systemd-hibernate-resume.service` so it can never roll the filesystem back
under a hibernated kernel. `CONFIG_HIBERNATION=y` in the Valve kernel
(`7.2.7-valve2`).

The `immutability-hibernate` graph node is what proves this end to end: it
hibernates, boots, and asserts the `boot_id` is unchanged and no reset ran.
It has never been run, so treat resume as designed-but-unproven until it has.

BIOS and controller firmware update at boot through
`jovian.devices.steamdeck.autoUpdate`. A BIOS flash can clear the Secure Boot
keys. `settings.boot.autoEnrollKeys` leaves signed key files on the ESP, and
systemd-boot enrolls them again the next time the firmware is in Setup Mode.

## Check on the device

The VM tests cover the LUKS, swapfile, hibernate and immutability path, but on
the architecture of whichever machine ran the graph — `immutability-hibernate`
declares no `arch` — not on Deck hardware. These still need the device:

- unl0kr with the DRM backend on the Valve kernel
- hibernate and resume on the Deck's drivers
- Sunshine KMS capture in Gaming Mode
- the SD card label and the semu paths in `STEAM-DECK-OLED.nix`
- `jupiter-biosupdate` with Secure Boot enabled
