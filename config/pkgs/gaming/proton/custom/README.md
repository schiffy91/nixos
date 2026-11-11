# proton-custom

`GE-Proton11-12` plus three small Wine bug fixes. The package fetches the exact
Wine tree GE built from, applies GE's own `wine-hotfixes` sequence in
`protonprep` order (their series change the wineserver protocol, so anything we
overlay must come from the same tree), applies the patches below, rebuilds only
the modules they touch as matched PE/Unix pairs (`ntdll`, `win32u`,
`winewayland.drv`), and overlays those files on the GE binary release. DXVK,
vkd3d-proton and everything else are GE's binaries. WineASIO is added for
Rocksmith.

`default.nix` exposes the tool to Steam through `programs.steam.extraCompatPackages`
and links it at `~/Games/proton/proton-custom-GE-Proton11-12`, the path the
Battle.net launcher uses as `PROTONPATH`.

## Patches

| Topic | Fixes |
|---|---|
| `wine-wayland-layered-windows` | Layered toplevels with constant alpha or a colour key uploaded a zero alpha byte and vanished (tooltips, splash screens). |
| `ntdll-delay-load` | `LdrResolveDelayLoadedAPI` wrote into a read-only `/guard:cf` delay-load IAT and crashed the module. |
| `win32u-load-driver-deadlock` | `load_desktop_driver()` waited forever on a stalled explorer desktop thread; the barrier is now bounded. |

Each patch is `git format-patch -s` output against the GE base and is meant to
go upstream to GE or Wine; the folder README explains symptom, root cause and
fix.

## Rebased onto GE-Proton11-12 on 2026-11-06

The Wine source and binary release are pinned together. The protonprep replay
includes the new pending patches in upstream order, including Wayland lock
ordering, HDR presentation, NVIDIA instance destruction and thread cleanup.
DXVK is updated from v3.1.1-134 to v3.1.1-174, whose release notes include
present timing fixes and an NVIDIA fence workaround.

Two local patches are no longer active because GE now covers their fixes:
`wine-wayland-focus` is replaced by the upstream patch that posts
`WM_CANCELMODE` without blocking the event dispatcher, and
`ntdll-system-thread-signals` is replaced by `em-fixups/0073`, which leaves the
libc thread pointer intact when a Unix-only system thread handles a signal.
Their patch files remain as historical references. The remaining three patches
are still applied to the new Wine tree.

## Rebased onto GE-Proton11-11 on 2026-11-02

All five patches survived unchanged. The upstream files they touch
(`win32u/driver.c`, `ntdll/loader.c`, `winewayland.drv/wayland_keyboard.c`,
`winewayland.drv/window_surface.c`) are byte-identical between the 11-7 and
11-11 Wine trees, and the GE patches the other two take their context from are
unchanged — `wine-wayland/0088` introduces the synchronous `WM_CANCELMODE` and
`0196` produces its final surroundings; `em-fixups/0030` adds
`start_system_thread` — so each one applies at a line offset with no fuzz and
no rejects. Nothing in 11-11 covers them: no GE
patch touches `load_desktop_driver`, `LdrResolveDelayLoadedAPI` or the system
thread's libc thread pointer.

The protonprep replay below gained GE's new Wine work: the
`winedevice-Default_Drivers` staging set, the seven ACE anti-cheat game
patches, the `mta-certificate`, `llvm-libc` and `bluetooth-le` hotfix
directories, and the new `pending` patches (Direct2D Tint, schannel
`SECBUFFER_MISSING`, xwayland-satellite minimization, off-desktop minimized
paint, APC/exit-state ordering, the freed handle table in `close_handle`, the
SRW lock wake, and the Switch Pro/DualSense Edge identities).

Watch 11-11's winewayland HDR work (`em-fixups` 0066, 0069, 0070, 0071) if
HDR output behavior changes on this host.

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
(`LogPixels`), written by `wine-prefix-dpi` from `config/pkgs/gaming/wine/default.nix`:
the Battle.net launcher sets it on its prefix before each start, and the Steam
default launch options run every game through `wine-prefix-dpi exec`, which
writes it into that game's prefix.

Battle.net follows the active KScreen output, including Sunshine's virtual
display. When that output's scale changes, the launcher restarts after any
running game closes, because Wine caches system DPI at process startup.
It waits for the previous prefix session to exit before changing the registry.

## Development

`make shell` enters the dev shell; `make setup` seeds `src/wine` from the
package's patched source and configures 64- and 32-bit build trees; `make
wayland`, `make win32u`, `make ntdll` rebuild one module and overlay it into a
dev copy of the compat tool that `make use` points Steam and the Battle.net
launcher at; `make restore` points them back at the store build. `make
wine-format-patch RANGE=...` exports commits from `src/wine` as patch files.
`make check-patches` and `make patch-lint` verify the series applies and is
well formed. `src/` is git-ignored scratch.

The dev shell supplies `proton-custom-dev`, `proton-custom-build`,
`proton-custom-install`, and `proton-custom-make` from `tools.nix`. Their BTRC
implementation lives in `tools/`; the same package is used by the Battle.net
test shell. `proton-custom-dev --help` lists launch, capture, and source commands.
