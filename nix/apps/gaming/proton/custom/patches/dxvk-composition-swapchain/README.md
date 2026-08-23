# DXVK composition swap chains

## Status
Active DXVK series. Applications using DirectComposition can create DXGI
composition swap chains through `CreateSwapChainForComposition`; DXVK should
service that API through its normal presenter path instead of relying on an
application profile.

## Fix
Enable DXVK's composition swap-chain path by default and expose a small private
interface that Wine's `dcomp.dll` can use to bind the swap chain to the real
DComp target window. After that bind, presentation goes through DXVK's normal
Wayland WSI presenter instead of a Wine-side image copy.

Composition swap chains are paced with the compositor when the application
requests immediate presentation. They also preserve the previous frame in the
newly exposed back buffer after rotation, which keeps damaged-only paints from
showing stale hover remnants or video/banner flicker.

When a bound composition swap chain changes size, DXVK resizes the private
composition child HWND to the new swap-chain extent. That keeps KDE snap, tile,
and maximize transitions from leaving the Wayland child surface at the previous
size.

DXVK also shows private child composition targets on the first present instead
of during the DComp commit that binds the swap chain. This keeps hidden host
windows from becoming visible before CEF has submitted a frame, which was the
reproducible post-login white-window failure.

If a composition swap chain presented before DComp bound it to a target, or if
DComp later rebinds it to a freshly created host window, DXVK replays the
current back buffer into the new target immediately. Battle.net has static
login/loading surfaces that may not submit another frame after the bind; the
replay keeps those first opens and download-time host churn from staying black
or white until unrelated UI activity happens.

The native Wayland WSI surface for those private child HWNDs is advertised as
opaque even when the DXGI composition swap chain uses premultiplied alpha. The
Wine DComp bridge keeps the original DXGI alpha metadata for visual placement,
but it does not composite the full DComp tree itself; letting KWin blend the
child surface by that alpha can erase otherwise rendered Chromium frames against
Battle.net's black parent window.

The tail of the series adds opt-in debug trace points around composition
`Present`, target binding, target showing, and WSI image acquire. Set
`DXVK_TRACE_COMPOSITION_SWAPCHAINS=1` with `DXVK_LOG_LEVEL=debug` to distinguish
"no present after bind" from "present succeeded but the target stayed black".

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series. The Wine-side series consumes only the private composition bind
interface; GPU presentation, color management, synchronization, and HDR remain
inside DXVK's presenter.
