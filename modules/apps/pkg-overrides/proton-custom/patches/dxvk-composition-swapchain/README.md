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

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series. The Wine-side series consumes only the private composition bind
interface; GPU presentation, color management, synchronization, and HDR remain
inside DXVK's presenter.
