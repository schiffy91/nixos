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

The patch also exposes a small private composition-swap-chain interface that
Wine's `dcomp.dll` can use to bind the dummy composition swap chain to the real
DComp target window. After that bind, presentation goes through DXVK's normal
Wayland WSI presenter instead of a Wine-side image copy.

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series. The Wine-side series consumes only the private composition bind
interface; GPU presentation, color management, synchronization, and HDR remain
inside DXVK's presenter.
