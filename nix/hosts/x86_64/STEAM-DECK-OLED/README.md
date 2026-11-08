# Steam Deck OLED

`STEAM-DECK-OLED.nix` enables Jovian's hardware support and Gaming Mode, a
touchscreen LUKS prompt inside Plymouth, hibernation on every sleep, Secure Boot key
re-enrollment and Sunshine with session-aware capture. Its `let` block holds the device
data: disk, SD card label and LAN.

Decky Loader runs from Jovian; install plugins from its store. The non-Steam
shortcuts in `settings.apps.steam.shortcuts` are written to every Steam profile
at boot, before Steam starts. Entries can specify `{ exe = "..."; icon = "..."; }`;
icon changes preserve existing shortcut IDs, tags, controller settings and
play history. Other shortcuts in Steam are left alone. Semu uses ES-DE's icon.

Both physical hosts import `../desktop-apps.nix` for Cider, mpv, the development
CLIs, zsh, Mullvad, OpenVPN 3, Discord, Solaar and Logitech receiver support. Racing-wheel
tools remain on FRACTAL-NORTH. Both use the same custom Proton package. Battle.net
opens Blizzard's installer on first launch if its Windows client is absent.
Proton launches use XWayland in Gaming Mode and native Wayland in a Wayland
desktop session; Gamescope lacks the subcompositor protocol Wine requires.

Sunshine uses KMS in Gaming Mode and a KWin virtual display in Plasma. The virtual
display follows the streaming client's resolution and restores the local display
when streaming stops. KRFB is installed with that feature.

The SD card holds the entire emulation library, including emulator settings and
mod links; the emulator programs live in the Nix store on the internal SSD. Its
mount path is derived from `settings.users.admin.username` and the SD label.
Semu creates relative mod links and ownership records so moving the card to a
different mount point keeps them usable. After importing older state with broken
links, close affected emulators and run `semu mods repair --target steam-deck`.
This reconnects only broken links for entries present in the configured mods
library, leaves working foreign links and real files alone, and records old link
targets in each emulator's `semu-mod-repairs.tsv`.

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
2. Reboot and unlock with Plymouth's on-screen keyboard. It appears automatically
   when a touchscreen is present and no typing keyboard is attached. With a USB
   keyboard attached, tap the screen to reveal it. The first tap only reveals
   the keyboard. Rendering and touch coordinates follow the panel's DRM rotation.
   Verify touch input on the actual Deck; a VM cannot validate its digitizer.
3. Prepare Secure Boot separately using the checks below. Do not enroll keys
   before verifying an independent key backup and signed external recovery.

## Secure Boot preparation

See [firmware settings](BIOS.md) for the observed BIOS state, how to enter Setup
Utility, and the separate firmware and kernel requirements for IOMMU.

The audited OLED F7G0114 firmware was in Setup Mode with Secure Boot disabled.
That does not establish a recovery toggle in its firmware interface. Before
`sudo nixosctl secure-boot enable`, prepare unique keys, independently back them
up on encrypted storage and verify restoration, and boot a signed external
recovery device. Use only our own keys, with no `--microsoft` or publicly shared
private keys. Keep `settings.boot.autoEnrollKeys = false` during this preparation,
including the first signed maintenance configuration, until external recovery
and deliberate enrollment are ready. The [independent key backup procedure](../../../../tests/secure-boot/backup/README.md)
uses root-only `nixosctl secure-boot keys` transfers and verifies restoration on
Fractal North's separate encrypted disk without enrolling or rotating keys.

Preparation verified on 2026-11-07: the Deck has unique keys in `/var/lib/sbctl`,
with an independently verified backup and restoration on Fractal North under
`/var/lib/nixos/secureboot-backups/STEAM-DECK-OLED`. A recovery UKI signed by
those keys is prepared on Fractal at
`/var/lib/nixos/recovery/STEAM-DECK-OLED/nixos-recovery.efi`; its signature was
verified, but it has not been booted from a physical recovery USB. The Deck
remains in Setup Mode with Secure Boot disabled. External recovery and firmware
enrollment/removal checks remain required before enabling it.

`nixosctl secure-boot disable` currently switches to an unsigned configuration
only after firmware Secure Boot is already disabled. It does not remove the
platform key. The authenticated removal candidate is `sbctl reset`, using the
current platform key; it must be verified on the Deck with working recovery
before relying on it. Do not assume a BIOS toggle or a firmware flash will clear
keys.

Disable automatic enrollment **before** deliberate platform-key removal.
`settings.boot.autoEnrollKeys` configures systemd-boot to enroll keys from
`/boot/loader/keys/auto` whenever firmware enters Setup Mode. A maintenance boot
must keep signed boot/recovery images, set `secure-boot-enroll off`, disable the
enrollment preparation service, and remove stale enrollment assets from the
ESP. Turning off a Nix option alone does not prove existing files were removed.
Otherwise the next boot can undo the intended key removal. Retained recovery
generations must also disable enrollment; an older auto-enrolling generation
can recreate the assets. Lanzaboote protects the currently booted generation,
so limiting the number of entries while still running an old generation is not
sufficient. Boot maintenance first, rebuild its menu, and verify that only safe
maintenance/recovery entries remain. Verify both boots before removing the key.
Firmware SecureBoot mode describes the current boot, so check its new state
after a restart, not immediately after the runtime key update.

The `secure-boot-removal` VM graph scenario checks this sequence under OVMF,
including rejection of the wrong private key, repeated boots without automatic
re-enrollment, explicit re-enrollment, and retained signed recovery. It does not
validate the Deck firmware, independent backup, or a physical recovery USB.
See [sbctl's reset implementation](https://github.com/Foxboron/sbctl/blob/0.18/cmd/sbctl/reset.go)
and [systemd-boot enrollment policy](https://www.freedesktop.org/software/systemd/man/latest/loader.conf.html).
The boot-time firmware mode is specified in [UEFI 2.10 section 3.3](https://uefi.org/specs/UEFI/2.10_A/03_Boot_Manager.html).

## Desktop mode

The Deck has its own Plasma taskbar launchers. Workstation-only Chrome app IDs
live in the FRACTAL-NORTH host configuration. Plasma Keyboard is selected as
KWin's input method on the Deck, available from the taskbar's **On-Screen
Keyboard** launcher even without Steam. Automatic pop-up is disabled so it
does not cover Steam's keyboard when using **Steam+X**. Close one keyboard
before opening the other.

**Return to Gaming Mode** is available on the desktop, taskbar and application
menu. It uses `steamosctl` to switch sessions. A Deck-only KWin script keeps
Steam's separate desktop keyboard within the scaled display's work area while
preserving its requested height so the bottom row stays visible.

## Sleep and firmware

Steam and Plasma ask logind to suspend, and `settings.sleep.hibernateOnSuspend`
turns that into hibernation. With the current Deck configuration, a successful
hibernation resume asks for the LUKS passphrase; an aborted request still needs
a session lock. Valve [disabled Deck hibernation in the SteamOS 3.9.4 beta](https://steamcommunity.com/ogg/1675200/announcements/detail/694275728822242440)
(October 2026), citing driver bugs. Switching `settings.sleep.hibernateOnSuspend`
to `false` uses ordinary suspend and requires a working password lock first.

The Deck selects `settings.sleep.suspendMode = "deep"`. The pinned Valve
kernel [explicitly rejects s2idle on Jupiter/Galileo](https://github.com/Jovian-Experiments/linux/blob/7.2.10-valve1/kernel/power/suspend.c),
even when a forced `mem_sleep_default=s2idle` makes it appear selected in sysfs.
Other hosts retain their s2idle default. This corrects the suspend selection;
it does not establish reliable deep suspend on the physical Deck. Keep the
known-booted fallback and verify wake, input, graphics, audio and networking
before changing the daily hibernation policy.

Read-only journal inspection on 2026-11-07 confirmed both successful physical
hibernation/resume and prior failures: a freezer abort, and image writes aborted
by a pending wakeup (`-11`) with AMDGPU thaw errors (`-16`). Hibernation is
possible but not a reliable authentication boundary by itself. A failed sleep
request still needs password locking; do not treat an unlocked return as safe.

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
(`7.2.10-valve1`).

The `immutability-hibernate` graph node exercises this sequence in a VM: it
hibernates, boots, and asserts the `boot_id` is unchanged and no reset ran.
That scope does not establish hibernation or peripheral resume on the physical
Deck; those require separate hardware validation.

`jovian.devices.steamdeck.autoUpdate` runs the packaged BIOS/controller update
services. The BIOS updater applies version-specific rules: in the currently
pinned `jupiter-20261028.1` package, automatic installation is disabled for
OLED F7G0100–F7G0113, and F7G0114 has no newer target. Enabling the service therefore
does not guarantee an available BIOS update is installed automatically.

The updater runs native `h2offt` against a signed firmware payload and may stage
`EFI/Insyde/isflash.bin` for the next reboot. Jovian also supplies a packaged
fwupd BIOS remote. Neither path establishes that updates require Microsoft's
Secure Boot certificates or that a flash erases our keys. Verify update
compatibility and key retention on the physical Deck before relying on an
own-key-only firmware-update workflow; do not add vendor trust or force a flash
as a workaround. Source: [pinned BIOS updater](https://github.com/Jovian-Experiments/jupiter-hw-support/blob/jupiter-20261028.1/usr/bin/jupiter-biosupdate).

## Check on the device

The systemd-boot generation selector uses UEFI console mode `5`, documented by
NixOS as the Deck's horizontal mode. This runs before Linux and Plymouth;
`fbcon=rotate:1` and the touch keyboard cannot change its layout. Avoid `max`:
it selects the highest mode number, not the highest resolution. Check the menu
on the physical Deck after firmware updates, since a VM has different UEFI modes.

Plymouth owns both the splash and the LUKS prompt. Its patched `two-step` plugin
contains a Steam-style US touch keyboard; it does not run the Steam client in
the initrd. libinput watches touchscreen and keyboard hotplug events. The Deck's
firmware keyboard and controller interfaces do not count as attached typing
keyboards. Real USB keyboards suppress automatic display, while a screen tap
always reveals it. A touch preference persists across incorrect-passphrase retries.

The keyboard includes uppercase, punctuation, Space, Backspace, Caps and Unlock.
Its default Steam-style palette has a charcoal tray, dark character keys, black
action keys, muted shifted-symbol hints and blue pressed/active keys. Rendering
uses Plymouth's fonts, with no dependency on Steam assets. Keys and hit targets
share the same geometry in display-logical coordinates; width is 96% of the
screen capped at 1280 logical pixels, and five rows occupy its lower half. Labels
fit their key bounds. Very small displays still have physically small touch targets.
Touch coordinates use the same rotation as Plymouth's renderer, including the
Deck's portrait panel mounted sideways. Initrd module loading precedes Plymouth
startup so the graphics driver is available when the encrypted disk asks for its
passphrase. There is no separate unl0kr agent competing for the display.

`initrd-touch-unlock` and `initrd-touch-unlock-rotated` in `tests/graph.json` boot
real encrypted volumes with delayed graphics-driver loading. Kernel uinput
fixtures provide touchscreen and keyboard devices, and tests inspect screenshots
and enter a passphrase through libinput and Plymouth. The rotated test uses an
800×1280 virtual panel with a DRM orientation property. Small C tests cover the
visibility policy, all printable US characters, hit regions and coordinate rotation.
The full host boot tests cover the installer, Jovian kernel and other host modules.

### Hardware testing

Build a fresh host-specific ISO after changing the boot configuration. The
installer uses the system closure baked into that ISO; editing its source alone
does not replace the installation payload.

The live installer enables SSH for root with the admin's authorized public keys.
Once it has a network connection, use `ssh root@<installer-ip>` to inspect the
Deck and run tests from the build machine. Build on the desktop and transfer
artifacts to the Deck, then reboot and check the physical display and touch input.

Installer SSH is available after the live system boots. It does not provide
access to an installed system stuck at its LUKS prompt. Diagnosing that stage
remotely requires a separate diagnostic initrd with networking and SSH; a wired
USB Ethernet connection avoids depending on early-boot Wi-Fi support.

After a failed graphical unlock followed by USB-keyboard unlock, collect:

```sh
sudo journalctl -b -u plymouth-start.service -u systemd-ask-password-plymouth.service -u systemd-modules-load.service
sudo journalctl -b -k | rg -i 'amdgpu|drm|firmware|hid|touch|i2c'
ls -l /dev/dri /sys/class/drm
cat /proc/bus/input/devices
```

These distinguish missing firmware, early DRM selection, and missing touch
devices. A normal boot journal does not capture touch coordinates or screenshots; those
still require testing on the physical panel.

The VM tests cover the LUKS, swapfile, hibernate and immutability path, but on
the architecture of whichever machine ran the graph — `immutability-hibernate`
declares no `arch` — not on Deck hardware. These still need the device:

- Plymouth graphics and touch alignment with the DRM backend on the Valve kernel
- hibernate and resume on the Deck's drivers
- Sunshine KMS capture in Gaming Mode
- the SD card label and the semu paths in `STEAM-DECK-OLED.nix`
- `jupiter-biosupdate` with Secure Boot enabled
