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
also pass `amd_iommu=off`. The host module now filters that flag from the merged
kernel parameters while preserving the other Deck defaults. This change needs
a new boot; the running generation checked on 2026-11-08 still had the flag,
no IVRS table and no IOMMU groups. It has not yet been validated on hardware.

A first-hand [report in Valve's tracker](https://github.com/ValveSoftware/SteamOS/issues/2077)
shows IOMMU enabled alongside Secure Boot. It does not identify LCD versus OLED
or provide a NixOS configuration, so it establishes a working Deck report rather
than proof for this firmware/kernel combination.

There are also [Steam Deck enablement scripts](https://github.com/Smalls1652/steamdeck-iommu-fix)
and [owner configuration notes](https://github.com/lwbt/docs-steamdeck-notes/blob/main/FAQ.adoc#can-i-enable-iommu).
The script repository was last pushed in October 2022, before OLED. Its author
requires both a kernel-command-line change and the firmware IOMMU setting.
Those examples also select `iommu=pt`; this host does not copy that choice,
because [passthrough bypasses DMA translation](https://docs.kernel.org/admin-guide/kernel-parameters.html).

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

The Deck was in Setup Mode during preparation. The applicable route is signing the NixOS
boot images with this device's own keys and deliberately enrolling those keys
from Linux. The [DeckSecureBoot author's implementation notes](https://github.com/HiSkyZen/DeckSecureBoot#how-this-works)
describe enrollment activating enforcement without a BIOS switch. That project
ships shared keys; this repository uses its own unique keys instead.

Follow the existing [Secure Boot preparation](README.md#secure-boot-preparation):
the independent key backup and signed external recovery boot were verified on
2026-11-08, including unlocking and mounting the installed SSD from the USB.
Keep automatic enrollment off and use the explicit enrollment/removal commands. Neither a
BIOS reset nor a firmware update has been verified to clear enrolled keys here.

The explicit enrollment command successfully installed this Deck's own db, KEK
and PK on 2026-11-08. After reboot, `SecureBoot=1`, `SetupMode=0`, and the kernel
reported `Secure boot enabled`. The signed installed generation reached Gaming
Mode; `nixosctl secure-boot status` verified key ownership and boot signatures.
