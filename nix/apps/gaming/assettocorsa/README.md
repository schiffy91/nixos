# Assetto Corsa and the Logitech rig

Original Assetto Corsa is Steam **244210**. Competizione is **805550** and
does not get these mods. Steam owns the game download and DLC.

The flake pins Content Manager 0.8.2895.40668, public CSP 0.2.11, CM fonts,
and Logitech TrueForce driver/tools 0.42.1. AC uses the repo's shared
custom GE-Proton (currently GE-Proton11-7) and the shared DPI launcher,
which derives scaling from the declared primary display (240 DPI on FRACTAL).
AC keeps an XWayland/SDR environment for CM. GE's bundled AC fix installs .NET 4.8,
d3dx11_43, and d3dcompiler_47 on first launch. That first prefix creation
still needs network access and can take several minutes; the installer
recipe is pinned with Proton. Microsoft core fonts are supplied by Nix.

## First use

```sh
cd /etc/nixos
sudo nixos-rebuild switch --flake .#FRACTAL-NORTH-Standard-Boot
# Once Steam downloads finish, exit Steam:
nix run .#assetto-setup
```

Open Steam again. AC uses the same `proton-custom` compatibility tool as the other games.
Steam Input is disabled for AC by default. Press **Play**.
Content Manager opens instead of the stock launcher; the stock executable
is preserved. Steam's runtime command and arguments are retained. Both the
launcher override and Steam Input setting are applied by rebuild activation
(with Steam closed) and `assetto-setup`, using the same declaration.

To change controller emulation, set `settings.apps.assettocorsa.steamInput` to
`"disabled"` (default), `"default"` (follow Steam's global preference), or
`"enabled"`. It affects only original AC. This controls Steam's input
emulation, not AC's steering/pedal/gear bindings; those still need validation
in CM before saving a hardware-specific preset.

The wrapper sets CM's AC folder and links Steam's account-discovery file
into the prefix. It discovers separate Steam libraries automatically;
`assetto status` prints the selected paths. Under Settings → Content Manager →
Appearance, disable window transparency if popups are black. CM/CSP automatic
updates are disabled by the wrapper so the declared versions remain in use.
CM's unrelated settings and license data are preserved. Its original
`Values.data` is backed up as `Values.before-nix` before the first edit.

```sh
nix run .#assetto -- status
nix run .#assetto -- apply    # also happens automatically before each launch
nix run .#assetto -- restore  # restore overwritten originals; remove managed additions
```

`restore` leaves Steam's launch options alone. To return completely to the
stock launcher, restore first, set `settings.apps.assettocorsa.enable = false`,
exit Steam, and rebuild. While enabled, the next Play reapplies the pack.

Mod deployment records its files in the game's `.nix-assettocorsa/` directory.
It backs up preexisting files, removes retired managed files, preserves
unrelated mods, and refuses to overwrite locally edited managed files. Move
such an edited file aside, then apply again to return to the declaration.
It refuses an incomplete Steam download and symlinks in destination paths.
Changing the pack does not reset a Wine prefix, controls, saves or replays.

## Add cars, tracks, apps, or weather

Set `settings.apps.assettocorsa.mods` in a host module. Each package contributes
a directory tree relative to the AC root. ZIP, 7z and RAR can be unpacked
using the helper; inspect the archive to select the correct directory.
File collisions, including Windows case collisions, fail the Nix build.

```nix
{ pkgs, ... }:
let
  inherit (import ../../../apps/gaming/assettocorsa/lib.nix { inherit pkgs; }) archive;
in {
  settings.apps.assettocorsa.mods = [
    (archive {
      name = "my-track-1.0";
      src = pkgs.fetchurl {
        url = "https://AUTHOR/RELEASE/my-track-1.0.zip";
        hash = "sha256-REPLACE_WITH_PREFETCH_HASH";
      };
      # If the ZIP already contains content/tracks/..., omit both fields.
      subdirectory = "my_track";
      target = "content/tracks/my_track";
      # format = "7z"; # also supports RAR through 7z
    })
  ];
}
```

Get a URL's hash with `nix store prefetch-file --json 'URL'`.
For an authenticated or purchased download (Pure, preview CSP, paid cars),
download it from the author and use a fixed local source instead:

```nix
src = pkgs.requireFile {
  name = "Exact-Downloaded-Filename.zip";
  sha256 = "REPLACE_WITH_HASH";
  message = "Download this version from the author, then nix-store --add-fixed sha256 FILE";
};
```

Run `nix hash file --type sha256 --base32 FILE` for that hash and
`nix-store --add-fixed sha256 FILE` to supply it. Keep your own backup:
another machine needs the same archive. A licensed archive and configuration
are reproducible; an account entitlement is not something Nix can provision.
Do not commit paid archives to Git. CSP previews replace the base CSP package
through `settings.apps.assettocorsa.csp = archive { ... };`; adding one alongside
the public CSP in `mods` deliberately fails the collision check.

The starting pack provides CM and CSP's lighting, weather framework, graphics
extensions and bundled Lua apps. Choose further cars/tracks for the servers
you want to join, including their DLC dependencies. Pure and Shutoko/traffic
packs are optional next choices, not silently downloaded purchases.

## PRO wheel, pedals, and RS H-Shifter

This rig has the **PlayStation/PC PRO Racing Wheel** (`046d:c268`), PRO
Racing Pedals, and Playseat Trophy. The H-pattern accessory's product name
is **RS H-Shifter** (7+R); it is distinct from the sequential RS Shifter &
Handbrake. The wheel driver is enabled only on FRACTAL-NORTH through
`settings.apps.simracing.enable` and rebuilt with the selected kernel.

1. Connect the wheel in **PC / PRO mode**. Connect the pedals to the wheel
   base for the initial setup; the H-shifter can connect by USB directly to
   the PC or through the compatible wheel base. USB detection still needs
   checking on the real devices.
2. Open **Logi Wheel** (`logi-wheel-gui`) to verify detection and settings.
   `nix shell nixpkgs#usbutils nixpkgs#evtest` provides temporary diagnostics;
   `lsusb -d 046d:` and `evtest` show the kernel's devices and axis/button
   events. Use the wheel's on-device controls for initial strength and
   brake force. Start at low strength for the first force-feedback test.
3. In CM → Settings → Assetto Corsa → Controls, select Wheel, bind steering,
   throttle, brake, clutch, gears 1–7 and reverse, then enable H-shifter use.
   Match wheel rotation and game rotation (900° is a starting point).
   Verify the pedal bars at rest and full travel; invert axes only if needed.
   Use a manual-transmission car to test the clutch and shifter.
4. Save a named control preset. AC's Documents/Assetto Corsa directory lives
   inside its Steam Wine prefix, covered by Steam's existing persistence.
   The native Logi Wheel configuration and SDK data are persisted separately.

AC's launch wrapper invokes the pinned `logi-launch`, which selects the
driver's per-game FFB/telemetry behavior after GE's first-run prerequisites
are installed. Starting its relay earlier prevents the installers from
waiting for Wine to exit. Original AC has no native TrueForce;
the driver's telemetry-based haptics are a separate optional enhancement.
Its setup app can install the relay in the game prefix after first launch.
Standard steering/pedals/FFB should be validated before tuning haptics.
No Logitech proprietary SDK files are included. ACC/EVO's native TrueForce
path needs files from a Windows G HUB install, as described upstream.

The physical USB bindings, load-cell calibration, force feedback, and
firmware state cannot be proven from a Nix build. Firmware updates still
require Logitech-supported tooling. The pedals and shifter do not require
installing a Windows G HUB driver into the AC Wine prefix.

## TV

The existing host config remembers the Samsung **S89C** as `HDMI-A-2`:

```sh
nixosctl displays list
nixosctl displays enable S89C
nixosctl displays primary S89C
nixosctl displays layout
nixosctl audio list
```

After reconnecting, verify the connector and available modes in Plasma
Display Settings, enable the desired refresh rate, and choose the HDMI audio
sink. AC starts in SDR; establish working driving/FFB before HDR experiments.
CM's resolution/fullscreen setting must match the TV. The TV isn't enabled
automatically when absent, and the primary monitor is not disabled.

## Verification and sources

```sh
nix build .#checks.x86_64-linux.gaming-tools
nix build .#assetto .#assetto-setup
```

All repo-owned gaming programs and tests are BTRC under `btrc/gaming` and
`tests/unit/gaming.btrc`. Steam configuration and AC share library discovery
and VDF handling. All Wine callers share `wine-prefix-dpi`. The Nix modules
only build, package, and compose those tools. Upstream Proton and the BTRC
compiler may use Python internally; no Python application code is embedded
in these Nix modules.

Tests exercise install/update/removal/restore, manual edits, path escapes,
case collisions, incomplete downloads, separate Steam libraries, Steam Input
and ACC isolation, CM compression formats, DPI registry preservation, and the
actual composed launcher with spaces. Hardware and an actual race remain
separate acceptance checks.

- [Linux AC setup recipe](https://github.com/sihawido/assettocorsa-linux-setup)
- [Pinned CM release](https://github.com/gro-ove/actools/releases/tag/v0.8.2895.40668)
- [CSP releases](https://acstuff.club/patch/)
- [Logitech driver and NixOS module](https://github.com/mescon/logitech-trueforce-linux-driver)
- [Logitech RS H-Shifter announcement](https://ir.logitech.com/news/news-details/2026/Shift-Into-Realism--Introducing-the-Logitech-G-RS-H-Shifter/default.aspx)

PSVR2 research is deferred until the monitor/wheel setup is working.
