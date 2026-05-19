# Real DirectComposition implementation (CEF accelerated rendering)

## Symptom
CEF-hosted apps (Battle.net launcher, Discord, Electron games) either
render with the slow GDI fallback or refuse to render at all under wine.
`d3d11`-backed compositor scenes produce a frozen / black launcher area.

## Root cause
Wine's `dcomp.dll` stubbed every COM entry point with `E_NOTIMPL`.  CEF's
compositor detects this, walks back to the GDI path (in the best case)
or just gives up on Chromium's composited UI tree (in the worst case).

## Fix
Implement the COM surface CEF actually uses:

* `IDCompositionDesktopDevice` — created from a D3D11 device.
* `IDCompositionVisual2` — sceneGraph node holding content.
* `IDCompositionSurface` — exposes a DXVK swap chain to the visual.
* `IDCompositionTarget` — bound to the host HWND.

`Commit()` snapshots the bound DXVK swap chain's current back buffer
through `IDXGIVkInteropSurface` and forwards it to the windowing driver
via a custom `WM_WAYLAND_COMPOSE_ATTACH` message, on a ~60 Hz
internal ticker so we present at compositor cadence without busy-spinning.

The driver-side bridge that consumes the dma-buf lives in patch 06.

## Dependencies
Pairs with:
* `05-dxgi-composition-swapchain` — `dxgi.CreateSwapChainForComposition`
  fallback so DXVK can create the swap chain in the first place.
* `06-winewayland-subsurface-bridge` — driver-side `wl_subsurface`
  presenter that consumes the dma-buf.

## Affected upstream
`dlls/dcomp/{Makefile.in, device.c}`.
