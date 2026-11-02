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

You need a USB-C hub with a keyboard and Ethernet, and the NixOS minimal ISO on
a USB drive. Hold Volume Down and press Power for the boot menu.

1. Boot the ISO and clone this repository to `/etc/nixos`.
2. Build the CLI:

   ```sh
   cd /etc/nixos
   NIX_CONFIG='experimental-features = nix-command flakes' nix-shell --run 'make build'
   ```

3. Run `sudo build/nixosctl install --format`, choose `STEAM-DECK-OLED` and set
   the password. It becomes both the LUKS passphrase and the account password;
   `nixosctl change-password --full-disk-encryption-only` later changes only the
   passphrase.
4. Reboot and type the passphrase on the touchscreen. unl0kr has no rotation, so
   its keyboard follows the panel's native portrait orientation.
5. The firmware ships in Setup Mode. Run `sudo nixosctl secure-boot enable --microsoft`.

## Sleep and firmware

Steam and Plasma ask logind to suspend, and `settings.sleep.hibernateOnSuspend`
turns that into hibernation, so every wake goes through the passphrase. Valve
disabled hibernation on the Deck in the SteamOS 3.9.4 beta (October 2026),
citing driver bugs. If resume misbehaves, set `settings.sleep.hibernateOnSuspend
= false;` in the host file to go back to suspend.

BIOS and controller firmware update at boot through
`jovian.devices.steamdeck.autoUpdate`. A BIOS flash can clear the Secure Boot
keys. `settings.boot.autoEnrollKeys` leaves signed key files on the ESP, and
systemd-boot enrolls them again the next time the firmware is in Setup Mode.

## Check on the device

The VM tests cover the LUKS, swapfile, hibernate and immutability path on
aarch64. These still need the hardware:

- unl0kr with the DRM backend on the Valve kernel
- hibernate and resume on the Deck's drivers
- Sunshine KMS capture in Gaming Mode
- the SD card label and the semu paths in `STEAM-DECK-OLED.nix`
- `jupiter-biosupdate` with Secure Boot enabled
