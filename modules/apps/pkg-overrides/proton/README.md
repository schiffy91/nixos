# scwhine-proton Battle.net Wayland GPU path

This directory packages `GE-Proton10-34` with a small, explicit Wine patch
series for Battle.net on native Wayland. The goal is an upstream-quality base
that keeps Chromium/CEF on the D3D11/DXGI/DirectComposition path instead of
papering over launcher failures with CPU compositing fallbacks.

## Package Shape

`package.nix`:

1. Fetches Valve Wine at `1729f00e17e879f98f9df1f2bca86bc5d21a65df`, matching
   the Wine tree used by `GE-Proton10-34`.
2. Applies the GE-Proton wine-wayland hotfix series.
3. Applies only the explicit `activePatchSeries` list, in Nix list order.
4. Builds the patched `dcomp.dll`, `dxgi.dll`, and `winewayland.drv` artifacts.
5. Overlays only those artifacts on top of the GE-Proton binary tarball.

`default.nix` installs the package through `programs.steam.extraCompatPackages`
and also keeps
`~/.local/share/Steam/compatibilitytools.d/scwhine-GE-Proton10-34` as a symlink
to the exact Nix store build, because the standalone `battlenet` wrapper uses
that path as `PROTONPATH`.

The build never glob-applies `patches/*/*.patch`. Patch folders are grouped by
upstream topic/PR candidate; the `0001/0002` filenames inside a folder are just
the `git format-patch -s` commit order for that topic.

## Active Patch Series

| Topic | Patch | Status |
|---|---|---|
| `wine-wayland-roundtrip` | `0001-winewayland.drv-Avoid-second-init-roundtrip.patch` | Active. Avoids a blocking second Wayland init roundtrip. |
| `wine-wayland-layered-windows` | `0001-winewayland.drv-Hook-UpdateLayeredWindow.patch` | Active. Hooks `pUpdateLayeredWindow` only. |
| `wine-wayland-status-notifier` | `0001-winewayland.drv-Add-StatusNotifierItem-tray-support.patch` | Active. Adds SNI tray support through Wine's Wayland driver. |
| `dcomp-wayland-gpu-present` | `0001..0004` | Active. Implements a DComp/DXGI/winewayland dma-buf presentation path for D3D11 DirectComposition clients. |
| `dxvk-battlenet-composition` | `0001-dxgi-Enable-dummy-composition-swapchain-for-Battle.n.patch` | Active. Makes Battle.net's DXVK `dxgi.dll` accept DirectComposition swap-chain creation. |

The previous numbered prototype directories were removed. Useful lessons from
them were folded into the topic folders above; keeping old failed attempts in
tree made the review story worse.

## Live Findings

The earlier cleanup series starts Battle.net under native Wayland and registers
a Plasma SNI tray item. The previous Qt `bad_array_new_length` startup crash was
fixed by the Wayland roundtrip change.

Stock Chromium D3D11 GPU compositing under Wine Wayland presented a black right
pane. These attempts did not fix it:

- `dxgi.enableDummyCompositionSwapchain = True`
- `--disable-direct-composition`
- `--disable-gpu-memory-buffer-compositor-resources`
- the deleted DComp/DXGI/subsurface prototype stack

Forcing Chromium's ANGLE desktop backend painted the launcher but was not
acceptable:

- `--use-angle=desktop`
- `--high-dpi-support=1`
- `--force-device-scale-factor=<Wine LogPixels / 96>`

`--use-angle=desktop` tells Chromium's ANGLE layer to try the desktop OpenGL
backend instead of the normal D3D path. Live post-login testing showed this is
not the final GPU-composited solution: Battle.net still spawned renderer
processes with `--disable-gpu-compositing`, and earlier `--use-gl=desktop` /
`--use-angle=gl` probes failed to create Chromium's shared GPU context.

The `battlenet` wrapper now defaults to the D3D11/DXGI/DComp path.
`BATTLE_NET_ANGLE_BACKEND` and `BATTLE_NET_DISABLE_GPU_COMPOSITING=1` remain
available as diagnostics, not as the expected user experience. `battlenet-x11`
remains the control path.

## Build

From this worktree:

```bash
nix build --print-out-paths --impure --expr 'let flake = builtins.getFlake "/home/alexanderschiffhauer/nixos-bnet-wayland"; in builtins.elemAt flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.config.programs.steam.extraCompatPackages 0' -o /tmp/bnet-scwhine-core-result
```

Full system test:

```bash
nixos-rebuild test --flake /home/alexanderschiffhauer/nixos-bnet-wayland#FRACTAL-NORTH-Secure-Boot
```

## Live Test

Capture loader and systray logs before changing behavior:

```bash
WINEDEBUG=+loaddll,+module,+systray PROTON_LOG=1 battlenet 2>&1 | tee /tmp/bnet-load.log
```

Confirm that these load from the rebuilt `scwhine-GE-Proton10-34` store path:

- `winewayland.so`
- `winewayland.drv`
- `dcomp.dll`
- `dxgi.dll`

Useful checks:

```bash
busctl --user --list | rg 'StatusNotifierItem|StatusNotifierWatcher'
rg -n 'Loaded L".*(winewayland|win32u|dcomp|dxgi|winevulkan)' "$HOME/steam-battlenet.log"
spectacle -b -n -o /tmp/bnet-wayland.png
```

Expected Wayland result:

- Battle.net launches without the Qt startup crash.
- The CEF login or launcher content paints instead of remaining black.
- The Battle.net command line does not include `--use-angle=desktop` or
  `--disable-gpu-compositing` unless explicitly requested for diagnostics.
- DComp/DXGI logs show composition swap-chain creation and dma-buf presentation.
- A StatusNotifierItem appears in the session bus.
- Tray Activate and ContextMenu D-Bus calls return promptly.
- `battlenet-x11` still launches as a control.

## Patch Hygiene

Active patches should be regenerated from real commits with:

```bash
git format-patch -s
```

Before promoting new material into the active series:

- Keep patch content ASCII-only.
- Keep project names such as `scwhine` out of upstream-bound patch content.
- Avoid copied private Wine struct layouts.
- Use configure/pkg-config plumbing for new library dependencies.
- Use `TRACE` for ordinary debug flow, `WARN` for unexpected recoverable states,
  and `ERR` only for real failures.
- Do not add broad patch globs or numbered topic directories back to
  `package.nix`.

## Next Design Work

The active DComp/DXGI/winewayland series is intentionally organized as a first
reviewable cut, not the last architecture discussion. The likely durable
upstream direction is a platform-neutral DComp/DXGI surface path that hands an
opaque presenter or shared resource to the user driver, with Wayland deciding
between dma-buf and fallback presentation behind that driver boundary.
