# Assetto Corsa and the Logitech rig

Original Assetto Corsa is Steam **244210**. Competizione is **805550** and
does not get these mods. Steam owns the game download and DLC.

The flake pins Content Manager 0.8.2895.40668, public CSP 0.2.11, CM fonts,
and Logitech TrueForce driver/tools 0.42.1. AC uses the repo's shared
custom GE-Proton (currently GE-Proton11-11) and the shared DPI launcher,
which derives scaling from the declared primary display (240 DPI on FRACTAL).
AC keeps an XWayland/SDR environment for CM. GE's bundled AC fix installs .NET 4.8,
d3dx11_43, and d3dcompiler_47 on first launch. That first prefix creation
still needs network access and can take several minutes; the installer
recipe is pinned with Proton. Microsoft core fonts are supplied by Nix.

AC's wrapper sets `WINE_DBGHELP_OLD_PDB=1`. The new PDB reader in this Wine
revision loses folded function aliases and public symbols when enumerating
AC's debug symbols, so CSP reports that it cannot find game functions.
Wine's existing compatible reader resolves them. This is scoped to AC/CM;
the shared Proton version and other games remain unchanged. See
[Wine's reader selection](https://github.com/ValveSoftware/wine/blob/46b29104e3741fe23bf5e2547196a253aab88c89/dlls/dbghelp/msc.c#L4010)
and the regression fixture in `tests/gaming/proton/pdb`.

## First use

```sh
cd /etc/nixos
sudo nixosctl update
# Once Steam downloads finish, exit Steam:
assetto-setup
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
assetto status
assetto apply    # also happens automatically before each launch
assetto restore  # restore overwritten originals; remove managed additions
```

`restore` leaves Steam's launch options alone. To return completely to the
stock launcher, restore first, set `settings.apps.assettocorsa.enable = false`,
exit Steam, and rebuild. While enabled, the next Play reapplies the pack.

Mod deployment records its files in the game's `.nix-assettocorsa/` directory.
It backs up preexisting files, removes retired managed files, preserves
unrelated mods, and preserves runtime edits while the declared file is unchanged.
When upgrading or removing a mod, edited files are archived under
`.nix-assettocorsa/edits/<sha256>/<relative-path>` before replacement.
It refuses an incomplete Steam download and symlinks in destination paths.
Changing the pack does not reset a Wine prefix, controls, saves or replays.

## Add cars, tracks, apps, or weather

Drop downloaded ZIP, 7z or RAR mods into `~/Games/Assetto Corsa`. The next
launch (or `assetto apply` with CM closed) detects cars, tracks and game-root
content automatically. An extracted `content/cars` or `content/tracks` tree
also works. Archives stay in this directory; extraction is cached by SHA-256.
Identical downloads and identical overlapping files are skipped. Different
versions that claim the same file stop deployment before changing the game;
keep only the version you want. Remove a download to uninstall its managed
content on the next apply. Runtime edits are archived before removal.

The directory is configured with a string, not a Nix path:

```nix
settings.apps.assettocorsa.contentDirectory = "~/Games/Assetto Corsa";
```

Use `null` to disable local content imports. The flake still pins CM, CSP,
fonts and Proton; local downloads remain outside the Nix store. Back up
this directory to reproduce your collection on another machine.

The in-game CSP UI follows the same display-derived DPI as the shared Wine
launcher, capped to CSP's documented 100–200% range. For example, FRACTAL-NORTH's
250% desktop scale produces a 200% game UI. Override it with
`settings.apps.assettocorsa.uiScale = 1.5;` for 150%, or use `null` to follow DPI.
Setup updates only the relevant keys in Documents/Assetto Corsa/cfg/extension/gui.ini
and saves the original as `gui.ini.before-nix` when that file already exists.
It enables CSP's replacement race and pause menus because the original menus
ignore that scale setting. CSP marks the replacement main menu experimental; legacy or third-party
apps can still have their own independent sizing. Game rendering resolution is
unchanged.

On Plasma, `settings.apps.assettocorsa.followLauncher = true;` (the default)
starts each race on Content Manager's current monitor. Move CM to the TV
before starting a race to use that screen. A small KWin script waits for Wine's
startup geometry requests to settle, places the game once, then leaves manual
window moves alone. It does not change the desktop's primary monitor or AC's resolution.
Set this option to `false` to leave placement entirely to the game.

Saved CM presets can live beside the downloads and remain writable:

```nix
settings.apps.assettocorsa.presets = [{
  name = "Video Settings/FRACTAL-NORTH.cmpreset";
  source = "~/Games/Assetto Corsa/FRACTAL-NORTH.cmpreset";
}];
```

The name is relative to CM's `Presets` directory. Setup moves an existing
preset to the configured location and replaces it with a symlink; CM's saves
then update that file. Conflicting copies are preserved and reported.
The setting accepts CM presets from any category. `Video Settings` above is
the category where CM saved this particular file, not a restriction of the
setting. A saved preset contains that category's data, not all CM preferences.
Remove an entry (or set `presets = [];`) to copy the latest contents back
into Wine and stop linking. The external copy is retained. These files are
ignored by the mod importer. Keep a backup of `~/Games/Assetto Corsa` too.

Control presets use `.ini` files in AC's Documents directory rather than CM's
`.cmpreset` directory. Declare them using `name = "Controls/FRACTAL-NORTH.ini";`
and an external source such as `~/Games/Assetto Corsa/Controls/FRACTAL-NORTH.ini`.
The same migration, conflict detection and unlink behavior applies. Audio and
other CM presets keep their actual category, for example `Audio Settings/...`.

Additional wheel bindings are ordinary records in
`settings.apps.assettocorsa.controlBindings`:

```nix
[
  { action = "__CM_START_SESSION"; button = 0; }
  { action = "__CM_TO_PITS"; button = 2; modifier = 7; }
]
```

`joystick` defaults to 0, and `modifier` defaults to null. A null `button`
clears that action's wheel binding. Setup writes these
bindings to the active controls and declared control presets, preserving axes,
device GUIDs, force feedback, keyboard bindings and other actions. It records
both joystick and button IDs for modifiers. The first originals are backed up
as `.before-nix-bindings`; later setup never overwrites those backups. Declared
actions are reapplied on launch, so change their records in Nix to customize
them. Removing a declaration stops enforcing it; it does not reset controls.
`brakeGamma = null` preserves the saved brake curve; FRACTAL declares 1.0 for
a linear response from its load-cell brake. Other pedal calibration stays saved
in the external control preset. Adjust the physical brake force on the wheel.

The complete host binding declaration is in
`nix/hosts/x86_64/FRACTAL-NORTH/assettocorsa.nix`. The current profile uses
direct buttons and a dial-operated Wheel Menu, enabled with
`settings.apps.assettocorsa.wheelMenu.enable`. It replaces the earlier
L2/Share modifier layout; those buttons now have ordinary actions.

| Control | Action |
|---|---|
| Paddles | Shift up/down |
| Cross | Select in CSP menus |
| Square | Camera |
| Circle | Handbrake while driving; resume from pause or exit instant replay on release |
| Triangle | Headlights |
| R2 / L2 | DRS / KERS |
| Options | Pause/resume |
| Share | Save replay clip |
| R3 / L3 | Pit limiter / flash headlights |
| Left dial | Select Wheel Menu item |
| Right dial | Adjust selected value |
| Right dial press | Activate a labelled menu command; otherwise show/hide Wheel Menu |
| Left dial press | Horn |
| D-pad | Saved look bindings while driving; CSP navigation in race/pit menus |

The left-select/right-adjust interaction follows GT7's documented MFD pattern
for supported replacement wheels, not a claim to reproduce Logitech's entire
factory GT7 mapping. See [GT7's MFD mapping](https://www.gran-turismo.com/hk/gt7/news/00_5412323.html).
Wheel Menu uses CSP's Lua app API; its small runtime adapter lives beside this
package in `wheel-menu/`. Deployment and preset management remain BTRC.

Wheel Menu exposes supported TC/TC2, ABS, brake bias, turbo, engine map cycling, H-shifter mode, engine braking,
hybrid recovery/delivery/MGU-H, wipers, beam selection, indicators/hazards,
pit strategy and game-provided pit options. It shows acknowledged car values,
respects locked pit options and does not adjust values in pause, replay or
pre-race menus. Fuel remaining is read-only. Neutral is an explicit press
command; FFB and seat/FOV adjustments use CSP's native commands. Cockpit/mirror
entries appear only while stationary, and a disappearing entry cannot cause
that frame's input to adjust a different setting. Mirror changes preserve the
other parameters and respect mirrors controlled by car scripts.

The starter and cutoff are held commands, available only when CSP's existing
engine-stalling option is enabled. This setup does not enable experimental
stalling implicitly. Release, selection changes and entering pause cancel a
held command; returning with the button still down does not restart it.
Circle cancellation covers pause and instant replay; it is not universal
Back in every menu. CSP's controller integration is enabled for replay too.

The software tests pass, and the earlier app version loaded in the real CSP
session and rendered in the actual race. Physical wheel input, the expanded
controls and complete pit/session flows still require acceptance testing.
See [control coverage](CONTROLS.md) for the
remaining work; this table is a binding reference, not a hardware test result.

Setup explicitly enables CSP controller navigation, pit-menu navigation and
`CONTROL/BINARY_MODIFIERS_FILTERING2`. The latter prevents a modified button
from also firing its normal action (for example wipers and TC together).
Declarations reject duplicate chords and assigning a standalone action to a
modifier. Native AC actions cannot use modifier fields; the modified commands
use CM/CSP's supported system bindings instead.

The driver documents the [DirectInput button indices](https://github.com/mescon/logitech-trueforce-linux-driver/blob/master/docs/BUTTON_MAPPING.md).
See also [CSP modifier filtering](https://github.com/ac-custom-shaders-patch/acc-extension-config/blob/master/config/general.ini)
and the installed `extension/config/data_manifest.ini` for action availability.
Wipers, indicators, starter, engine maps and hybrid systems depend on the car
and enabled CSP features. Binding a starter does not enable engine-stalling
physics; binding wipers does not install paid RainFX. Cars without ABS/TC/DRS
cannot acquire those features from a binding. Online races cannot be paused
for other participants.

Replay operation and extra car-specific animations still need acceptance
testing or further wheel access. The physical pedal
axes, H-shifter gears and FFB need an in-car check. CM's desktop settings
pages do not yet have a wheel-operated interface.

`settings.apps.assettocorsa.mods` remains available for optional pinned Nix
packages containing game-root trees. `settings.apps.assettocorsa.csp` can
replace the public CSP package with a licensed preview. Conflicting content
is rejected instead of silently choosing a version.

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
make -C tests gaming
nix build --no-link .#nixosConfigurations.FRACTAL-NORTH-Standard-Boot.config.system.build.assetto .#nixosConfigurations.FRACTAL-NORTH-Standard-Boot.config.system.build.assetto-setup
```

Gaming deployment programs and their tests are BTRC under `btrc/gaming` and
`tests/gaming/GamingTests.btrc`. The CSP app uses Lua and the KWin adapter uses
JavaScript, as required by their host APIs; their tests live under
`tests/gaming/assettocorsa`. Steam configuration and AC share library discovery
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
