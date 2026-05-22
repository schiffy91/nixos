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

The active profile also caps `d3d11.maxSharedResourceTier` at zero for
Battle.net. Chromium otherwise probes a D3DKMT shared-resource path Wine
does not implement completely, which can show up as missing or unstable CEF
surfaces while the launcher itself is still GPU accelerated.

The patch also exposes a small private composition-swap-chain interface that
Wine's `dcomp.dll` can use to bind the dummy composition swap chain to the real
DComp target window. After that bind, presentation goes through DXVK's normal
Wayland WSI presenter instead of a Wine-side image copy.

The local dithering patch is enabled only for this Battle.net test profile as a
reversible compositor-output experiment. It should not be treated as evidence
for system-wide gradient banding.

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series. The Wine-side series consumes only the private composition bind
interface; GPU presentation, color management, synchronization, and HDR remain
inside DXVK's presenter.
