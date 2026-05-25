# DXVK Battle.net composition swapchain

## Status
Active local DXVK series.  Battle.net loads DXVK's native `dxgi.dll`, not
Wine's builtin `dxgi.dll`, so the runtime `CreateSwapChainForComposition`
failure has to be fixed in DXVK's active code path.

## Fix
Add a DXVK application profile for `Battle.net.exe` that enables
`dxgi.enableDummyCompositionSwapchain`. DXVK already uses this option for
applications that need composition swap chains; carrying it as a profile keeps
the workaround in DXVK rather than in the launcher wrapper or per-prefix
`dxvk.conf`.

The active profile keeps Battle.net's D3D11 shared resource tier capped at
zero. Chromium otherwise advertises GPU shared-resource handles through CEF IPC
and then duplicates them across processes; Wine's current shared GPU resource
plumbing does not yet provide a real duplicable NT handle for that path.

The patch also exposes a small private composition-swap-chain interface that
Wine's `dcomp.dll` can use to bind the dummy composition swap chain to the real
DComp target window. After that bind, presentation goes through DXVK's normal
Wayland WSI presenter instead of a Wine-side image copy. Active composition
swap chains are paced with the compositor when the application requests
immediate presentation, which avoids UI-tearing and micro-stutter in launcher
surfaces. They also preserve the previous frame in the newly exposed back
buffer after rotation. CEF uses partial composition updates heavily; without
valid previous contents, damaged-only paints can show up as faint hover remnants
or video/banner flicker.

When a bound composition swap chain changes size, DXVK resizes the private
composition child HWND to the new swap-chain extent. That keeps KDE snap, tile,
and maximize transitions from leaving the Wayland child surface at the previous
size.

Composition swap chains deliberately skip DXVK's present-wait frame queue and
eager next-image acquire. Battle.net tears down transient CEF/DComp popup
surfaces during login, and waiting for Wayland WSI present completion on those
surfaces can deadlock popup destruction before the main CEF content browser is
created.

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series. The Wine-side series consumes only the private composition bind
interface; GPU presentation, color management, synchronization, and HDR remain
inside DXVK's presenter.
