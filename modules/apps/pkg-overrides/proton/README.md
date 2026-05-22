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
4. Builds the patched Wine artifacts touched by the active series:
   `dcomp.dll`, `dxgi.dll`, `explorer.exe`, `winewayland.drv`,
   `winevulkan`, and `win32u`, including the matching PE/Unix halves.
5. Builds the patched DXVK `dxgi.dll` and `d3d11.dll` artifacts.
6. Overlays only those artifacts on top of the GE-Proton binary tarball.

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
| `wine-wayland-status-notifier` | `0001..0003` | Active. Adds SNI tray support, callback polish, and explorer-to-driver icon snapshots for updates/tooltips. |
| `dcomp-wayland-gpu-present` | `0001..0014` | Active. Implements the minimal DComp object model Battle.net uses and binds composition swap chains/surfaces to Wayland-backed host HWNDs. |
| `dxvk-battlenet-composition` | `0001..0004` | Active. Enables Battle.net composition swap chains in DXVK, compositor pacing, dithering experiment, and a shared-resource-tier cap. |

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

## Promotion Build

From this worktree:

```bash
nix build --print-out-paths --impure --expr 'let flake = builtins.getFlake "/home/alexanderschiffhauer/nixos-bnet-wayland"; in builtins.elemAt flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.config.programs.steam.extraCompatPackages 0' -o /tmp/bnet-scwhine-core-result
```

This is a promotion gate for packaging correctness. It is not part of the
Battle.net rendering/debug loop.

## Fast Local Iteration

The full Nix package path is intentionally reproducible, but it is too slow for
Battle.net UI experiments. Keep mutable source and build trees in this project
directory, ignored by git:

```text
modules/apps/pkg-overrides/proton/src/wine
modules/apps/pkg-overrides/proton/src/wine64
modules/apps/pkg-overrides/proton/src/wine32
modules/apps/pkg-overrides/proton/src/dxvk
```

Use the local dev compat tool for hot loops through the project Makefile:

```bash
nix-shell /etc/nixos/modules/apps/pkg-overrides/proton/dev-shell.nix
cd /etc/nixos/modules/apps/pkg-overrides/proton
make setup
make smoke SECONDS=5
```

The smoke log should say `scwhine DEV GE-Proton10-34 (Wayland SNI)`, proving
that the existing `battlenet` wrapper is launching through the writable dev
copy at:

```text
~/.local/share/Steam/compatibilitytools.d/scwhine-GE-Proton10-34-dev
```

Once local Wine or DXVK build directories are configured, copy fresh artifacts
into that dev tool without a Nix rebuild:

```bash
make overlay-wine
make overlay-dxvk
```

The overlay commands also accept the reproducible Nix outputs directly:

```bash
wine_out=$(nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in (pkgs.callPackage /etc/nixos/modules/apps/pkg-overrides/proton/package.nix { inherit (pkgs) makeWrapper rsync unzip; }).passthru.wineArtifacts')
dxvk_out=$(nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in (pkgs.callPackage /etc/nixos/modules/apps/pkg-overrides/proton/package.nix { inherit (pkgs) makeWrapper rsync unzip; }).passthru.dxvkArtifacts')
./bin/bnet-dev overlay-wine --wine "$wine_out"
./bin/bnet-dev overlay-dxvk --dxvk "$dxvk_out"
```

For Wine changes, seed mutable build trees from the exact patched Nix source
and run hot loops against individual Wine targets:

```bash
make setup-wine
make loop TARGETS='dlls/dcomp/all' SECONDS=3
```

The helper keeps separate 64-bit and i386 Wine build trees so the local build
uses the same WoW64 shape as the Nix package. `overlay-wine` also updates
Explorer in GE-Proton's default prefix and in the live Battle.net prefix;
otherwise the SNI snapshot/update path can compile correctly but still run
stale Explorer code. A warm `dlls/dcomp/all` edit loop rebuilds and overlays
both DLLs, smoke-launches Battle.net, then kills leftover Wine/container
processes in roughly ten seconds.

For the fastest manual loop, edit the materialized Wine source directly and
rebuild only the touched target:

```bash
$EDITOR "$SCWHINE_WINE_SRC/dlls/dcomp/device.c"
make dcomp
make shot DELAY=8
```

The local `src/` tree is an ignored workspace. It exists so patches can be
materialized into real source for development; it is not the committed source
of truth. Once a change works, turn the relevant local source diff back into a
small topic patch under `patches/<topic>/` and keep `package.nix` as the
explicit series manifest. `make setup-wine` initializes `src/wine` as a local
git repo with a baseline commit after the current patch series is applied, so
working changes can be inspected and exported without guessing:

```bash
make wine-status
make wine-diff
git -C "$SCWHINE_WINE_SRC" commit -am 'dcomp: describe the tested fix'
make wine-format-patch RANGE='-1 HEAD'
```

For screenshot-driven checks, use:

```bash
BATTLE_NET_FORCE_SCALE=2.5 ./bin/bnet-dev capture --delay 45 --output /tmp/bnet-wayland.png
./bin/bnet-dev cleanup
```

Return to the pinned Nix store compat tool with:

```bash
./bin/bnet-dev restore
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
- DComp/DXGI logs show composition swap-chain creation, target binding, and
  DXVK presenter creation. Cold login starts can show a black client area for
  a few seconds before CEF's first painted frame; use a 45-second capture when
  checking for persistent black-window regressions.
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
