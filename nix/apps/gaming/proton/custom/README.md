# proton-custom

This directory packages the custom Proton compat tool. It starts from
`GE-Proton11-7`, applies an explicit Wine/DXVK patch series, and overlays
only the rebuilt artifacts on top of the GE binary release. The primary current
consumer is Battle.net on native Wayland, but the package is intentionally kept
generic enough to be the one custom Proton build used by Steam games too.

## Package Shape

`package.nix`:

1. Fetches Valve Wine at `46b29104e3741fe23bf5e2547196a253aab88c89`, matching
   the Wine tree used by `GE-Proton11-7`.
2. Applies GE-Proton's complete `wine-hotfixes` set in `protonprep` order
   (wine-wayland, the SNI patch, the child-rendering series, the remaining
   em-fixups). Those series change the wineserver protocol and wined3d, which
   the overlaid DLLs must match; every patch has to apply or the build fails.
3. Applies only the explicit `activePatchSeries` list, in Nix list order.
4. Builds the patched Wine artifacts touched by the active series:
   `dcomp.dll`, `dxgi.dll`, `explorer.exe`, `winewayland.drv`,
   `winevulkan`, `win32u`, `ntdll`, and `wow64win`, including the matching
   PE/Unix halves.
5. Builds the patched DXVK `dxgi.dll` and `d3d11.dll` artifacts.
6. Overlays only those artifacts on top of the GE-Proton binary tarball.

`default.nix` installs the package through `programs.steam.extraCompatPackages`
and also keeps
`~/.local/share/Steam/compatibilitytools.d/proton-custom-GE-Proton11-7` as a symlink
to the exact Nix store build, because the standalone `battlenet` wrapper uses
that path as `PROTONPATH`.

The build never glob-applies `patches/*/*.patch`. Patch folders are grouped by
upstream topic/PR candidate; the `0001/0002` filenames inside a folder are just
the `git format-patch -s` commit order for that topic.

## Active Patch Series

| Topic | Patch | Status |
|---|---|---|
| `wine-wayland-focus` | `0001` | Active. Sends GE's `WM_CANCELMODE` on keyboard leave as a notify message so the Wayland reader thread never blocks and the message is never dropped. |
| `wine-wayland-layered-windows` | `0001` | Active. Bakes premultiplied constant alpha into layered SHM uploads only when the compositor lacks `wp_alpha_modifier_v1`; GE-Proton11 ships the `pUpdateLayeredWindow` hook itself. |
| `ntdll-delay-load` | `0001` | Active. Makes a read-only delay-load IAT (`/guard:cf` `.didat`) writable before patching, serialized on the loader lock. |
| `dcomp-wayland-gpu-present` | `0001..0019` | Active. Implements the minimal DComp object model Battle.net uses with a sound COM lifetime (children hold the device, targets ref 1), direct-binds the first unplaced first-level swap chain to the target HWND, keeps placed/clipped/surface visuals on hidden host HWNDs torn down per commit, accumulates ancestor clips, and preserves virtual-surface content across resizes. |
| `win32u-load-driver-deadlock` | `0001` | Active. Bounds the `WM_NULL` desktop-driver readiness barrier so a stalled explorer desktop thread cannot hang every client. |
| `dxvk-composition-swapchain` | `0001..0008` | Active. Fixes the resize extent bug, enables DXGI composition swap chains with a private DComp bind interface, compositor pacing, async host show/resize, preserved contents across buffer rotation, first-present host visibility, retained-content replay after target rebinds, and trace-level composition logging (`DXVK_LOG_LEVEL=trace`). |

Dropped at the GE-Proton11-7 rebase, now covered by the base tree:

| Topic | Covered by |
|---|---|
| `wine-wayland-status-notifier` | GE em-fixups `0001-winewayland-add-SNI-tray-icons-and-native-context-me` (win32u `sni.c`, incl. the wow64win thunk); icons come from the app's pixmap, so `WINE_SNI_ICON_NAME` is gone |
| unbounded `vkWaitForPresentKHR` on hidden windows | GE child-rendering `0066-winevulkan-Skip-present-waits-when-the-window-is-not` plus the 3 s stall recovery in win32u; this was the Battle.net white-window deadlock (login window hidden for destruction while DXVK's frame thread waited on its FIFO present, GPU and viz threads queued behind the presenter lock, UI thread blocked in `DestroyWindow`) |

Dropped at the GE-Proton11-5 rebase, now covered by the base tree:

| Topic | Covered by |
|---|---|
| `wine-wayland-popups` | GE-Proton wine-wayland `0031-winewayland-Implement-xdg-popup-for-unmanaged-window` |
| `wine-wayland-layered-windows` `Hook-UpdateLayeredWindow` | GE-Proton wine-wayland `0014-winewayland.drv-Add-WAYLAND_UpdateLayeredWindow` |
| `win32u-shared-gpu-resource` | Proton 11 `win32u` implements `NtGdiDdDDIOpenResource`/`OpenResource2`/`QueryResourceInfo` |

Dropped in the 2026-08 series review as functional no-ops:

| Topic | Why |
|---|---|
| `wine-wayland-roundtrip` | `wl_display_roundtrip_queue` already flushes and dispatches; the patch only reverted GE wine-wayland `0142`'s third roundtrip |
| `wine-wayland-layered-windows` `Handle-fully-zero-alpha` | never returned its opaque case, and win32u's shape copy zeroes those pixels after upload anyway |
| `dxvk-composition-swapchain` `Allow-limiting-shared-resource-tier` | option had no consumer |

## Runtime Scope

The current series starts Battle.net under native Wayland, keeps Chromium/CEF on
the D3D11/DXGI/DComp path, and registers a Plasma StatusNotifierItem. DComp
identity first-level swap-chain wrappers bind directly to the target HWND, while
placed, clipped, nested, or surface-backed visuals keep native host HWNDs.
Target windows get child clipping when visuals are backed by host HWNDs, so
parent Qt/CEF repaints do not cover hosted composition content.

DXVK preserves composition swap-chain contents across buffer rotation, paces
composition presents with the compositor, keeps private composition child
windows sized to the swap-chain extent when a host is required, and exposes
those child surfaces to Wayland as opaque WSI targets. Wine owns the DComp
object model, Win32 lifetime, popup placement, and tray icon bridge.

The installed `battlenet` wrapper has no CPU-compositing fallback or ANGLE
backend override. App-specific diagnostics live in `nix/apps/gaming/battlenet/tests`,
not in the desktop launcher.

## Promotion Build

From this worktree:

```bash
nix build --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; in builtins.elemAt flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.config.programs.steam.extraCompatPackages 0' -o /tmp/bnet-proton-custom-core-result
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
`nix/apps/gaming/rocksmith/default.nix` and `nix/apps/gaming/steam.nix`. This package only
provides the WineASIO DLL payload.

## Fast Local Iteration

The full Nix package path is reproducible, but it is too slow for UI debugging.
Keep mutable source and build trees in this project directory, ignored by git:

```text
nix/apps/gaming/proton/custom/src/wine
nix/apps/gaming/proton/custom/src/wine64
nix/apps/gaming/proton/custom/src/wine32
nix/apps/gaming/proton/custom/src/dxvk
```

Use the local dev compat tool for hot loops through the project Makefile:

```bash
nix-shell /etc/nixos/nix/apps/gaming/proton/custom/dev-shell.nix
cd /etc/nixos/nix/apps/gaming/proton/custom
make setup
make status
```

The dev copy lives at:

```text
~/.local/share/Steam/compatibilitytools.d/proton-custom-GE-Proton11-7-dev
```

Once local Wine or DXVK build directories are configured, copy fresh artifacts
into that dev tool without a full Nix package rebuild:

```bash
make overlay-wine
make overlay-dxvk
```

The overlay commands also accept the reproducible Nix outputs directly:

```bash
wine_out=$(nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in (pkgs.callPackage /etc/nixos/nix/apps/gaming/proton/custom/package.nix { inherit (pkgs) makeWrapper rsync unzip; }).passthru.wineArtifacts')
dxvk_out=$(nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in (pkgs.callPackage /etc/nixos/nix/apps/gaming/proton/custom/package.nix { inherit (pkgs) makeWrapper rsync unzip; }).passthru.dxvkArtifacts')
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

The committed patches are the source of truth. The ignored `src/` checkout is a
generated workspace for fast iteration only:

```bash
make source-drift
make reset-source-drift
```

`source-drift` must pass before treating a build as production material.

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

Battle.net repro/debug loops live in `nix/apps/gaming/battlenet/tests`, not
here. Use that Makefile when exercising the launcher.

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
nix build --no-link --print-out-paths --impure --expr 'let flake = builtins.getFlake "git+file:///etc/nixos"; pkgs = import flake.inputs.nixpkgs { system = "x86_64-linux"; config.allowUnfree = true; }; in pkgs.callPackage /etc/nixos/nix/apps/gaming/proton/custom/package.nix { inherit (pkgs) makeWrapper rsync unzip; }'
```

Expected Wayland result:

- Battle.net launches without the Qt startup crash.
- The CEF login and launcher content paint through D3D11/DXGI/DComp.
- The Battle.net command line does not include `--use-angle=desktop` or
  `--disable-gpu-compositing`.
- DComp/DXGI logs show composition swap-chain creation, target binding, and
  DXVK presenter creation.
- A StatusNotifierItem appears in the session bus (GE's SNI implementation).
- Restarting the launcher while the Agent is busy (an update in progress)
  paints the main window; the login-to-main switch must not deadlock.
- `make test-hidden-present TOOL=...` passes. `tests/dcomp-hidden-present`
  binds a D3D11 composition swap chain to a window through
  `DCompositionCreateDevice2`, presents FIFO frames, hides the window with a
  present in flight and releases the swap chain. GE-Proton11-5 hangs there
  (DXVK's frame thread never leaves `vkWaitForPresentKHR` for an unmapped
  surface, which is the Battle.net white-window deadlock); GE-Proton11-7's
  bounded present wait returns and the test prints `PASS`.
- Tray Activate and ContextMenu D-Bus calls return promptly.

## Editing A Series

Patch files are the source of truth, so edit them as git history, not by hand.
Materialize a base tree once (Valve Wine plus GE's complete wine-hotfixes set
in `protonprep` order, and a throwaway commit with the `make_vulkan`/
`make_requests`/`make_specfiles`/`autoreconf` output), then give every topic
its own worktree seeded with `git am`. The GE-Proton11-7 base lives on
`base-117` in `src/rework/wine` (tag `valve-117` is the pristine Valve commit)
and on `dxvk-117` in `src/rework/dxvk`; the topic branches are `small-117`,
`dcomp-117` and `composition-117`:

```bash
git -C src/rework/wine worktree add ../wine-dcomp-117 -b dcomp-117 base-117
```

Fix commits in place (`git commit --fixup` + `GIT_SEQUENCE_EDITOR=true git
rebase -i --autosquash base`), compile the touched modules from a configured
`build64/` inside the worktree, then regenerate the topic with
`git format-patch -o patches/<topic> base..HEAD` and update `package.nix`.
Worktrees checked out with `core.fileMode=false` lose exec bits: `chmod +x
configure` before configuring. `src/` is git-ignored; `src/rework` is
disposable.

## Patch Hygiene

Active patches should be regenerated from real commits with:

```bash
git format-patch -s
```

Before promoting new material into the active series:

- Keep patch content ASCII-only.
- Keep project names such as `proton-custom` out of upstream-bound patch content.
- Avoid copied private Wine struct layouts.
- Use configure/pkg-config plumbing for new library dependencies.
- Use `TRACE` for ordinary debug flow, `WARN` for unexpected recoverable states,
  and `ERR` only for real failures.
- Keep `package.nix` as an explicit series manifest.

## Next Design Work

The active DComp/DXGI/winewayland series is intentionally organized as a first
reviewable cut, not the last architecture discussion. The likely durable
upstream direction is a platform-neutral DComp/DXGI surface path that hands an
opaque presenter or shared resource to the user driver, with Wayland deciding
between dma-buf and fallback presentation behind that driver boundary.
