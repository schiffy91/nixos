# DXVK composition swap chains

## Status
Active DXVK series. Applications using DirectComposition can create DXGI
composition swap chains through `CreateSwapChainForComposition`; DXVK should
service that API through its normal presenter path instead of relying on an
application profile.

## Series
1. `[d3d11] Use new extent when resizing swap chain surface` fixes a pristine
   bug: `ChangeProperties` handed the presenter the old swap chain extent, so
   the preferred extent always lagged one resize behind. Win32 surfaces mask
   this because Wine pins the surface's current extent to the client rect;
   surfaces that leave the extent to the application do not.
2. `[dxgi] Bind composition swap chains to DComp windows` enables the
   composition swap-chain path by default (which subsumes the per-game
   profiles that only turned it on) and adds the private
   `IDXGIVkCompositionSwapChain` interface. Wine's `dcomp.dll` binds a swap
   chain to the window hosting its visual through
   `SetCompositionTarget(HWND target, HWND dispatch)`; a null target returns
   the swap chain to its dummy window. The IID
   `4765d18a-eba0-40bd-a730-7f5f3d915c1f` and the vtable layout
   (`QueryInterface`, `AddRef`, `Release`, `SetCompositionTarget`) are the
   ABI shared with Wine. Only swap chains created without a window expose
   the interface. Binding retargets the surface factory before destroying
   the presenter's resources, and the factory keeps one dummy window alive
   for the lifetime of the swap chain instead of destroying and recreating
   it around binds from a foreign thread. The WSI surface stays opaque; the
   DXGI alpha mode is kept only for the compositor's placement logic.
3. `[d3d11] Pace composition swap chains with the compositor` clamps the sync
   interval of any bound composition swap chain to at least one, so
   immediate presents from UI toolkits do not flicker under the desktop
   compositor.
4. `[d3d11] Keep composition target windows sized to swap chains` resizes a
   child host window to the swap chain extent on bind and on
   `ResizeBuffers`, using `SWP_ASYNCWINDOWPOS` because the host belongs to
   the dcomp commit thread and a synchronous cross-thread `SetWindowPos`
   from the render thread is a hang hazard.
5. `[d3d11] Preserve composition swap chain contents` copies the presented
   frame into the newly exposed back buffer after rotation while bound, so
   clients that only repaint damaged regions do not show stale content.
6. `[d3d11] Show composition targets on first present` shows a child host
   with `ShowWindowAsync` on the first present after it is bound, instead of
   during the commit, so hidden hosts do not become visible before the
   client has submitted a frame.
7. `[d3d11] Replay composition content after target binds` re-presents the
   last presented frame into a newly bound target if the application has
   presented before. The replay reads the last back buffer (where rotation
   leaves the presented frame) without rotating again. It runs on the
   commit thread through the regular present path, flushing the immediate
   context mid-frame and bumping the frame id; swap chains with a frame
   latency waitable object are skipped.
8. `[d3d11] Trace composition present paths` logs binds, presents of bound
   swap chains, host shows, replays and acquire results at DXVK's trace log
   level. Run with `DXVK_LOG_LEVEL=trace` to distinguish "no present after
   bind" from "present succeeded but the target stayed black". The state is
   snapshotted once per present under a single lock, and the level is
   checked before any message is built.

## Upstream
This belongs in DXVK, separate from the Wine `dcomp-wayland-gpu-present`
series. The Wine-side series consumes only the private composition bind
interface; GPU presentation, color management, synchronization, and HDR remain
inside DXVK's presenter.
