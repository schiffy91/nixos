# dxgi.CreateSwapChainForComposition HWND_MESSAGE fallback

## Symptom
CEF's compositor calls `IDXGIFactory2::CreateSwapChainForComposition`
(no HWND - the swap chain is presented through DComp) and receives
`DXGI_ERROR_INVALID_CALL` from DXVK.  Without that swap chain, the
compositor cannot create its presentation backing and the launcher
renders blank.

## Root cause
DXVK rejects `CreateSwapChainForComposition` calls unless
`dxgi.enableDummyCompositionSwapchain = True` is set in `dxvk.conf`.
That option is awkward to require for every CEF-hosted application and
is easy to miss when packaging.

## Fix
Synthesize a hidden `HWND_MESSAGE` owner in
`dxgi.CreateSwapChainForComposition` so the underlying
`CreateSwapChainForHwnd` call succeeds without the DXVK opt-in.  Apps
that already set the DXVK option are unaffected - the synthesized HWND
is only used as a placeholder for DXVK's HWND-based path.

The resulting swap chain is then driven by the `dcomp.dll` Commit
ticker added in patch 04.

## Dependencies
Pairs with `04-dcomp-direct-composition`.

## Affected upstream
`dlls/dxgi/factory.c`.
