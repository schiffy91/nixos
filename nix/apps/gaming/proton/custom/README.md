# proton-custom

`GE-Proton11-7` plus five small Wine bug fixes. The package fetches the exact
Wine tree GE built from, applies GE's own `wine-hotfixes` sequence in
`protonprep` order (their series change the wineserver protocol, so anything we
overlay must come from the same tree), applies the patches below, rebuilds only
the modules they touch as matched PE/Unix pairs (`ntdll`, `win32u`,
`winewayland.drv`), and overlays those files on the GE binary release. DXVK,
vkd3d-proton and everything else are GE's binaries. WineASIO is added for
Rocksmith.

`default.nix` exposes the tool to Steam through `programs.steam.extraCompatPackages`
and links it at `~/Games/proton/proton-custom-GE-Proton11-7`, the path the
Battle.net launcher uses as `PROTONPATH`.

## Patches

| Topic | Fixes |
|---|---|
| `wine-wayland-focus` | GE's `WM_CANCELMODE` on keyboard leave is sent synchronously from the Wayland reader thread; a busy window stalled all input. Sent as a notify message instead. |
| `wine-wayland-layered-windows` | Layered toplevels with constant alpha or a colour key uploaded a zero alpha byte and vanished (tooltips, splash screens). |
| `ntdll-delay-load` | `LdrResolveDelayLoadedAPI` wrote into a read-only `/guard:cf` delay-load IAT and crashed the module. |
| `ntdll-system-thread-signals` | `PsCreateSystemThread` threads never stored their libc thread pointer, so every 32-bit CEF renderer faulted on SIGQUIT at exit and left a core dump. |
| `win32u-load-driver-deadlock` | `load_desktop_driver()` waited forever on a stalled explorer desktop thread; the barrier is now bounded. |

Each patch is `git format-patch -s` output against the GE base and is meant to
go upstream to GE or Wine; the folder README explains symptom, root cause and
fix.

## Dropped on 2026-09-20

The DirectComposition emulation (`dcomp-wayland-gpu-present`, 19 patches, the
DXVK `dxvk-composition-swapchain` series, and `win32u-managed-swapchain-clip`)
is gone. It existed so CEF's DComp path would render Battle.net. Stock GE
refuses `CreateSwapChainForComposition`, CEF falls back to a plain window swap
chain, and that renders correctly, including after a game hides and re-shows
the launcher, which is exactly where the emulated path went black: the
toplevel's own buffer was never repainted with a hole over the dmabuf
subsurface. The `tests/dcomp-hidden-present` regression test went with it.

Earlier drops, now covered by GE itself: the StatusNotifierItem tray
(`em-fixups/0001`), bounded present waits on hidden windows (child-rendering
`0066`), xdg_popup for transient windows, the `pUpdateLayeredWindow` hook, and
D3DKMT shared resources.

## DPI

Nothing here scales UIs. Like Windows, the DPI lives in the prefix registry
(`LogPixels`), written by `wine-prefix-dpi` from `nix/apps/gaming/proton/dpi.nix`:
the Battle.net launcher sets it on its prefix before each start, and the Steam
default launch options run every game through `wine-prefix-dpi exec`, which
writes it into that game's prefix.

## Development

`make shell` enters the dev shell; `make setup` seeds `src/wine` from the
package's patched source and configures 64- and 32-bit build trees; `make
wayland`, `make win32u`, `make ntdll` rebuild one module and overlay it into a
dev copy of the compat tool that `make use` points Steam and the Battle.net
launcher at; `make restore` points them back at the store build. `make
wine-format-patch RANGE=...` exports commits from `src/wine` as patch files.
`make check-patches` and `make patch-lint` verify the series applies and is
well formed. `src/` is git-ignored scratch.
