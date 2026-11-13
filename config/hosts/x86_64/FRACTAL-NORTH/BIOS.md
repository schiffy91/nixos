# FRACTAL-NORTH Firmware Settings

ASUS ProArt X670E-CREATOR WIFI, Ryzen 9 7950X. Verified against BIOS 3902
(AGESA ComboAM5 PI 1.3.0.1b Patch A, 2026-07-16). Menu paths follow the ROG
STRIX X670E-series BIOS manual (E20544); the ProArt shares the same AGESA CBS
tree, so confirm names on screen if a menu moved.

Enter setup with `Del` at POST or `systemctl reboot --firmware-setup`.

## Required Settings

| Setting | Path | Value | Why |
|---|---|---|---|
| Secure Boot | Boot → Secure Boot → OS Type | `Windows UEFI Mode` | Enforcement on. Keys are ours (Lanzaboote/sbctl via `nixosctl secure-boot`); never use "Install Default Secure Boot keys" |
| Secure Boot Mode | Boot → Secure Boot → Secure Boot Mode | `Custom` | Lets `sbctl` own PK/KEK/db; `Standard` re-enrolls vendor keys |
| fTPM | Advanced → AMD fTPM configuration → Firmware TPM switch | `Enable Firmware TPM` | LUKS TPM2 keyslot (PCR 7+12) and fwupd HSI-1. Leave "Erase fTPM NV for factory reset" `Disabled` |
| IOMMU | Advanced → AMD CBS → IOMMU | `Enabled` | DMA remapping; also needed for `Pre-boot DMA protection` |
| TSME | Advanced → AMD CBS → UMC Common Options → DDR Options → DDR Security → TSME | `Enabled` | Transparent memory encryption; fwupd `Encrypted RAM`. Small (~1-3%) memory-latency cost. Leave `CPU Common Options → SMEE` on `Auto` |
| Thunderbolt / USB4 Support | Advanced → AMD PBS → Thunderbolt / USB4 Support | `Enabled` | Pro Display XDR |
| Thunderbolt / USB4 Security Level | Advanced → AMD PBS → Thunderbolt / USB4 Security Level | `No Security` (leave) | The board only offers `No Security` / `USB4 controller only`; the second may drop PCIe tunneling the XDR path needs. DMA protection comes from the OS IOMMU (see README) |
| BIOS Image Rollback Support | Tool → BIOS Image Rollback Support | `Disabled` (optional) | NIST SP 800-147 flash policy. ASUS already blocks rollback below 3702 |

## Curve Optimizer (Undervolt)

Path: Ai Tweaker → Precision Boost Overdrive → Curve Optimizer (or Advanced →
AMD Overclocking → Precision Boost Overdrive → Curve Optimizer; confirm on
screen). Set `Curve Optimizer` to `Per Core`, sign `Negative` on every core.
Change nothing else: PBO limits, scalar (1.0) and max CPU boost override (+0,
5850 MHz) stay at their defaults, as tested.

| Core | Linux CPUs | Magnitude |
|---|---|---|
| 0 | 0, 16 | 10 |
| 1 | 1, 17 | 6 |
| 2 | 2, 18 | 11 |
| 3 | 3, 19 | 11 |
| 4 | 4, 20 | 5 |
| 5 | 5, 21 | 11 |
| 6 | 6, 22 | 11 |
| 7 | 7, 23 | 11 |
| 8 | 8, 24 | 10 |
| 9–15 | 9–15, 25–31 | 11 |

Cores 0–7 are CCD0 and cores 8–15 are CCD1. If the BIOS lists each CCD as
"Core 0–7", then CCD1 Core N is core N+8 here.

Qualified at runtime (ryzen_smu) on 2026-11-11 to 2026-11-13. Per-core SSE and AVX2 sweeps,
2 h + 2 h multicore SSE/AVX2, 60 load-transition blocks, 8 h idle, and 2 h of
7-Zip/x265 output-checked load all passed. An interleaved stock comparison
measured about +1.5% throughput and 1–3.7% less energy per operation. Not
tested in a full WoW session. Evidence lives in
`~/.cache/cpu-co-qualification-2026-11-12-r6-cont4`.

If a crash, WHEA/MCE (`journalctl -k | grep -i mce`) or game instability shows
up, move the weakest cores 2 toward zero first: 4, 1, 0, 8.

## Checklist After Every BIOS Flash

ASUS resets AMD CBS (and sometimes Secure Boot) to defaults on flash. On
2026-05-18 both Secure Boot and TSME dropped silently.

1. Re-apply every row above and the Curve Optimizer table, save, reboot.
2. `sudo nixosctl secure-boot status` — expect enforcing with our keys.
3. `sudo nixosctl tpm2 status` — re-enroll (`sudo nixosctl tpm2 enable`) if the
   keyslot no longer unlocks.
4. `fwupdmgr security` — compare with the expected report below.
5. `cat /sys/bus/pci/drivers/ccp/*/tsme_status` — expect `1`.

## Expected `fwupdmgr security` Report

Target is **HSI:1** with the runtime suffix. Every row below shows its expected
mark; any other row that turns red, or a ✔ row here that turns red, is a
regression.

| Row | Expected | Reason |
|---|---|---|
| Platform secure boot (HSI-2) | ✘ Disabled | AMD PSB is an OEM fuse; ASUS consumer boards never burn it. Not user-fixable, and undesirable (vendor-locks the CPU) |
| SPI replay protection (HSI-3) | ✘ Not supported | Flash chip has no RPMC counter (`rpmc_spirom_available=0`) |
| Suspend-to-idle (HSI-3) | ✘ Disabled | Desktop firmware does not set the FACP `LOW_POWER_S0_IDLE` flag; `mem_sleep_default=s2idle` is already set on the kernel side |
| Processor rollback protection (HSI-4) | ✘ Disabled | AMD FAR/SPL fuse. ASUS's "BIOS Image Rollback Support" toggle is a different mechanism (fwupd #5261) |
| Encrypted RAM (HSI-4) | ✔ Encrypted | Only ✘ if TSME was reset — go fix it |
| Linux kernel lockdown | ✘ Disabled | This configuration does not enable kernel lockdown. |
| Linux swap | ✘ Invalid | False positive: btrfs swapfile `st_dev` is anonymous, fwupd cannot walk to the LUKS `dm-0` (fwupd #5123, wontfix). Swap is inside LUKS |
| Linux kernel | ✘ Tainted | `O` = NVIDIA out-of-tree, `W` = amdgpu WARN on the Raphael iGPU at boot |

Everything else must be ✔. `UEFI db: Not found` is correct for our own-key
Secure Boot setup.

## Firmware Inventory

| Component | Version | Update path |
|---|---|---|
| BIOS | 3902 (2026-07-16) | ASUS support page, EZ Flash. Not on LVFS |
| CPU microcode | 0x0a60120c | `hardware.cpu.amd.updateMicrocode` (nixpkgs) |
| Logitech Bolt receiver | MPR05.04_B0026 | `fwupdmgr update` |
| Pro Display XDR (TB) | 55.00 | `fwupdmgr update` (needs external power) |

## Known Non-Settings

- Kernel DMA protection for Thunderbolt: ASUS's DSDT omits `ExternalFacingPort`
  on the TB4 root port, so `/sys/bus/thunderbolt/devices/domain0/iommu_dma_protection`
  reads `0` and `boltctl` shows `security: none` regardless of BIOS. Mitigation is
  the OS keeping translated IOMMU domains (`cpu.nix`, no `iommu=pt`).
- AMD "enhanced atomics" 64-bit DMA corruption (fix pending upstream as of
  2026-09, confirmed only on Phoenix/Hawk Point/Strix APUs): if `journalctl -k`
  ever shows `csum failed` on `/dev/mapper/root` or `AMD-Vi: Event logged
  [IO_PAGE_FAULT]` under storage load, restore `iommu=pt` in `cpu.nix` and
  report it — Raphael would then be affected too.
