# proton-custom

This directory packages the custom Proton compat tool. It starts from
`GE-Proton10-34`, applies a small explicit Wine/DXVK patch series, and overlays
only the rebuilt artifacts on top of the GE binary release. The primary current
consumer is Battle.net on native Wayland, but the package is intentionally kept
generic enough to be the one custom Proton build used by Steam games too.

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
| `wine-wayland-status-notifier` | `0001..0004` | Active. Adds SNI tray support, callback polish, explorer-to-driver icon snapshots, and self-contained item lifetime handling. |
| `dcomp-wayland-gpu-present` | `0001..0016` | Active. Implements the minimal DComp object model Battle.net uses and binds composition swap chains/surfaces to Wayland-backed host HWNDs. |
| `win32u-load-driver-deadlock` | `0001` | Active. Bounds desktop-driver readiness waits. |
| `win32u-shared-gpu-resource` | `0001` | Active. Opens D3DKMT shared GPU resources used by composition paths. |
| `dxvk-battlenet-composition` | `0001..0007` | Active. Enables Battle.net composition swap chains in DXVK, compositor pacing, a shared-resource-tier cap, resize tracking, preserved contents across partial updates, and avoids blocking the app on composition present waits. |

The previous numbered prototype directories were removed. Useful lessons from
them were folded into the topic folders above; keeping old failed attempts in
tree made the review story worse.

## Live Findings

The current series starts Battle.net under native Wayland, keeps Chromium/CEF on
the D3D11/DXGI/DComp path, and registers a Plasma StatusNotifierItem. The
previous Qt startup crash was fixed by the Wayland roundtrip change.

The main rendering failure was stale or missing composition swap-chain content:
CEF relies heavily on partial updates, so rotating to an undefined back buffer
showed up as black panes, hover remnants, video/banner flicker, and resize
jitter. The active DXVK series now preserves the previous frame after rotation,
paces composition presents with the compositor, and keeps the private
composition child window sized to the swap-chain extent.

The installed `battlenet` wrapper has no CPU-compositing fallback or ANGLE
backend override. App-specific diagnostics live in `modules/apps/battlenet`, not
in the desktop launcher.

## Promotion Build

From this worktree:

```bash
nix build --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; in builtins.elemAt flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.config.programs.steam.extraCompatPackages 0' -o /tmp/bnet-scwhine-core-result
```

This is a promotion gate for packaging correctness. It is not part of the
Battle.net rendering/debug loop.

## WineASIO

WineASIO is packaged into `proton-custom` so Rocksmith can use the same compat
tool as everything else. The production package installs:

- `wineasio64.dll` and `wineasio64.dll.so` from nixpkgs `wineasio`.
- `wineasio32.dll` and `wineasio32.dll.so` from the existing Rocksmith payload,
  because nixpkgs currently only exposes the 64-bit output on this system.

Rocksmith still owns its prefix registration and low-latency launch flags in
`modules/apps/rocksmith.nix` and `modules/apps/steam.nix`. This package only
provides the WineASIO DLL payload.

## Fast Local Iteration

The full Nix package path is reproducible, but it is too slow for UI debugging.
Keep mutable source and build trees in this project directory, ignored by git:

```text
modules/apps/pkg-overrides/proton-custom/src/wine
modules/apps/pkg-overrides/proton-custom/src/wine64
modules/apps/pkg-overrides/proton-custom/src/wine32
modules/apps/pkg-overrides/proton-custom/src/dxvk
```

Use the local dev compat tool for hot loops through the project Makefile:

```bash
nix-shell /etc/nixos/modules/apps/pkg-overrides/proton-custom/dev-shell.nix
cd /etc/nixos/modules/apps/pkg-overrides/proton-custom
make setup
make status
```

The dev copy lives at:

```text
~/.local/share/Steam/compatibilitytools.d/scwhine-GE-Proton10-34-dev
```

Once local Wine or DXVK build directories are configured, copy fresh artifacts
into that dev tool without a full Nix package rebuild:

```bash
make overlay-wine
make overlay-dxvk
```

The overlay commands also accept the reproducible Nix outputs directly:

```bash
wine_out=$(nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in (pkgs.callPackage /etc/nixos/modules/apps/pkg-overrides/proton-custom/package.nix { inherit (pkgs) makeWrapper rsync unzip; }).passthru.wineArtifacts')
dxvk_out=$(nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in (pkgs.callPackage /etc/nixos/modules/apps/pkg-overrides/proton-custom/package.nix { inherit (pkgs) makeWrapper rsync unzip; }).passthru.dxvkArtifacts')
./bin/proton-custom-dev overlay-wine --wine "$wine_out"
./bin/proton-custom-dev overlay-dxvk --dxvk "$dxvk_out"
```

For Wine changes, seed mutable build trees from the exact patched Nix source and
build only the touched targets:

```bash
make setup-wine
make dcomp
make wayland
```

The helper keeps separate 64-bit and i386 Wine build trees so the local build
uses the same WoW64 shape as the Nix package. `overlay-wine` also updates
Explorer in GE-Proton's default prefix and in the live Battle.net prefix;
otherwise the SNI snapshot/update path can compile correctly but still run stale
Explorer code.

For the fastest manual loop, edit the materialized Wine source directly and
rebuild only the touched target:

```bash
$EDITOR "$PROTON_CUSTOM_WINE_SRC/dlls/dcomp/device.c"
make dcomp
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
git -C "$PROTON_CUSTOM_WINE_SRC" commit -am 'dcomp: describe the tested fix'
make wine-format-patch RANGE='-1 HEAD'
```

Battle.net repro/debug loops live in `modules/apps/battlenet`, not here. Use
that Makefile when exercising the launcher.

Return to the pinned Nix store compat tool with:

```bash
./bin/proton-custom-dev restore
```

## Production Check

Before activation, verify the Nix package or system graph:

```bash
nix build --dry-run --impure '/etc/nixos#nixosConfigurations."FRACTAL-NORTH-Secure-Boot".config.system.build.toplevel' --no-link
```

The package-only promotion build is:

```bash
nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in pkgs.callPackage /etc/nixos/modules/apps/pkg-overrides/proton-custom/package.nix { inherit (pkgs) makeWrapper rsync unzip; }'
```

Expected Wayland result:

- Battle.net launches without the Qt startup crash.
- The CEF login or launcher content paints instead of remaining black.
- The Battle.net command line does not include `--use-angle=desktop` or
  `--disable-gpu-compositing`.
- DComp/DXGI logs show composition swap-chain creation, target binding, and
  DXVK presenter creation. Cold login starts can show a black client area for
  a few seconds before CEF's first painted frame; use a 45-second capture when
  checking for persistent black-window regressions.
- A StatusNotifierItem appears in the session bus.
- Tray Activate and ContextMenu D-Bus calls return promptly.

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
