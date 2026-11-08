# Steam Deck OLED firmware settings

Read-only checks on 2026-11-07 found BIOS **F7G0114**, `SecureBoot=0`,
`SetupMode=1`, no ACPI `IVRS` table, no Linux IOMMU groups, and
`amd_iommu=off` on the running kernel command line. These are observations of
this Deck, not claims about every firmware revision.

To enter firmware setup, shut down, hold **Volume Up**, press **Power**, release
Volume Up after the chime, and select **Setup Utility** with the D-pad and A.
This is [Valve's documented procedure](https://help.steampowered.com/en/faqs/view/4c18-08b5-dec9-3af4).

## IOMMU

Look under **Advanced** for **IOMMU**. Enable it if present, save, and boot NixOS.
The option is reported in first-hand [Deck firmware observations](https://steamcommunity.com/app/1675200/discussions/0/3498761686676059197/);
Valve's support documentation does not specify its location for F7G0114, so an
absent option needs investigation rather than an assumed hidden-menu procedure.

The firmware change alone is insufficient: our pinned
[Jovian SteamOS defaults](https://github.com/Jovian-Experiments/Jovian-NixOS/blob/6d21db8ad755a73cc5df076e80fa80b714f555a6/modules/steamos/boot.nix)
also pass `amd_iommu=off`. A subsequent NixOS change must remove that flag while
preserving the other Deck defaults. Merely appending a contradictory flag is
not a reliable fix.

After firmware setup and a boot without that flag, check that `IVRS` exists,
Linux creates IOMMU groups, and the kernel reports translated DMA domains.
The [Linux AMD IOMMU driver](https://github.com/torvalds/linux/blob/master/drivers/iommu/amd/Kconfig)
requires firmware support; changing a kernel argument cannot manufacture the
missing firmware table. Exercise graphics, NVMe, SD, Wi-Fi and suspend/resume
before declaring the physical configuration working. A VM cannot validate the
Deck's firmware or device behavior.

## Secure Boot

Do not assume there is a visible stock-firmware Secure Boot switch. The author
of the [F7G0114 firmware-menu modification](https://www.stanto.com/steam-deck/unlocking-the-steam-deck-bios-secure-boot-options-higher-tdp-and-amd-cbs-pbs-for-ram-tweaking-with-a-flash/)
documents that Valve hides that menu. Flashing a modified BIOS is not part of
this configuration.

The Deck is already in Setup Mode. The applicable route is signing the NixOS
boot images with this device's own keys and deliberately enrolling those keys
from Linux. The [DeckSecureBoot author's implementation notes](https://github.com/HiSkyZen/DeckSecureBoot#how-this-works)
describe enrollment activating enforcement without a BIOS switch. That project
ships shared keys; this repository uses its own unique keys instead.

Follow the existing [Secure Boot preparation](README.md#secure-boot-preparation):
the independent key backup and signed recovery image have been prepared, but a
physical external-recovery boot has not been verified. Keep automatic enrollment
off until that check and a usable key-removal route are established. Neither a
BIOS reset nor a firmware update has been verified to clear enrolled keys here.
