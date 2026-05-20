# DXVK Battle.net composition swapchain

## Status
Active local DXVK series.  Battle.net loads DXVK's native `dxgi.dll`, not
Wine's builtin `dxgi.dll`, so the runtime `CreateSwapChainForComposition`
failure has to be fixed in DXVK's active code path.

## Fix
Add a DXVK application profile for `Battle.net.exe` that enables
`dxgi.enableDummyCompositionSwapchain`.  DXVK already uses this option for
applications that need composition swap chains; carrying it as a profile keeps
the workaround in DXVK rather than in the launcher wrapper or per-prefix
`dxvk.conf`.

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series.  The Wine-side series then consumes the DXVK swap chain through the
existing DXVK interop interfaces.
