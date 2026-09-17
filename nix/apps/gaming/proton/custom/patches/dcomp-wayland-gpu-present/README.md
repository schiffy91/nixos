# DComp/DXGI accelerated Wayland presentation

## Status
Active Wine series. This folder keeps the Wine half of the DirectComposition
work small: implement the DComp objects Battle.net's CEF/Chromium uses, then
bind composition swap chains and GPU-backed DComp surfaces to their target
HWND. DXVK owns the actual Wayland WSI presentation path.

## Organization
The series is split by upstream ownership:

1. `dcomp` (18 patches): implement the minimal D3D11-backed desktop device,
   target, visual, surface, virtual surface, surface factory and scroll
   behaviour needed to bind composition content, then refine how host
   windows are created, clipped, stacked and torn down.
2. `dxgi` (patch 0006): let `CreateSwapChainForComposition` create a swap
   chain on a message-only window for Wine's builtin DXGI path. The window
   is attached to the swap chain's private data so it is destroyed together
   with the swap chain (posted as `WM_CLOSE` when released on another thread).

The active path intentionally does not use `--disable-gpu-compositing`,
`--use-angle=desktop`, or a CPU `wl_shm` readback. Those remain useful
diagnostics, not acceptable defaults.

## Design
### DXVK contract
`dcomp.dll` talks to DXVK through the private `IDXGIVkCompositionSwapChain`
interface (IID `4765d18a-eba0-40bd-a730-7f5f3d915c1f`, vtable
`QueryInterface`/`AddRef`/`Release`/`void SetCompositionTarget(HWND target,
HWND dispatch)`). `dispatch` is always the DComp target HWND; `target` is
either the same HWND (content presented directly to the target) or a child
host HWND. DXVK treats `target != dispatch` as a child host and shows it on
the first present; `SetCompositionTarget(NULL, NULL)` unbinds. Keep the ABI
unchanged; the DXVK side is maintained in `dxvk-composition-swapchain`.

### Object lifetime
Visuals, surfaces, surface factories and targets each hold a reference on the
device; the device owns none of them and has no force-release loop. Targets
are created with a single reference and unlink themselves from the device's
target list when released, so a released target is never re-committed. A
target keeps its root visual alive and a visual keeps its linked children
alive (`AddVisual` retains, `RemoveVisual`/`RemoveAllVisuals`/parent release
drop the retained reference). `AddVisual` rejects cycles with `E_INVALIDARG`.
Content is unbound from DXVK before it is released or replaced by
`SetContent`.

### Commit and host teardown
`Commit` first marks every visual reachable from a target with the commit
generation, tears down the hosts of tracked visuals that were not reached
(unbinding their content first), and only then walks the trees and binds. So
Chromium's per-frame `RemoveAllVisuals` + `AddVisual` reorder keeps existing
hosts; only visuals that really left the tree lose theirs. Host windows are
created and destroyed on the committing thread: a visual released elsewhere
puts its host on an orphan list that the next commit (or the final device
release) destroys.

### Hosts, direct binding and stacking
Content at the target origin with no clip, a translation-only transform and
at most one level below the root is bound directly to the target HWND. Only
the first such swap chain in tree order (bottom-most) gets the target; later
eligible siblings are forced into hosts so two swap chains never fight for
one surface. Everything else (surfaces, placed, clipped, transformed or
nested content) gets a child host window at its accumulated placement.

Hosts are stacked in tree order: each commit remembers the host placed
before a host and restacks it (and every host after it) only when that
changed. Geometry, clip region and the `SetCompositionTarget` arguments are
cached on the visual so unchanged commits issue no `SetWindowPos`,
`SetWindowRgn` or DXVK retarget calls.

### Clipping
Clips accumulate down the tree in target coordinates: a visual's clip is
intersected with its ancestors' clips and applied to its descendants. The
host stays at the full content bounds and the intersection of the target
client area and the accumulated clip is applied with `SetWindowRgn`, so the
swap chain origin is never shifted.

### Surfaces
`IDCompositionSurface`/`IDCompositionVirtualSurface` draw into a D3D11
texture (`BeginDraw` returns `ID3D11Texture2D`, `IDXGISurface` or
`IDXGISurface1`, with the update offset of a partial rect). Commit copies a
dirty texture into a composition swap chain bound to the surface's host and
presents it. Hosts are created hidden; a surface host is shown once a
present actually happened (`presented`), so surfaces that were never drawn
do not show an empty host. `Resize` keeps the old content that still fits;
`Scroll` copies through a temporary texture. `EndDraw` without `BeginDraw`
returns `DCOMPOSITION_ERROR_SURFACE_NOT_BEING_RENDERED`; `SuspendDraw` and
`ResumeDraw` toggle the drawing state.

### Target window
`CreateTargetForHwnd` sets `WS_CLIPCHILDREN` on the target once, on the
caller's thread and outside the device lock, so Qt/CEF background repaints do
not cover the child hosts after DXVK presented Chromium content.

## Review Notes
* `IDCompositionVisual2Vtbl` and the other vtables are initialized
  positionally. The slot order in `include/dcomp.idl` IS the ABI: MSVC
  reverses adjacent overloads, which is why the `Set*Animation`/`Set*Object`
  entries come before their plain `Set*` counterparts. Verify the vtables
  against `include/dcomp.idl` slot by slot on every base bump.
* `Commit`, `SetContent`, `AddVisual`, `RemoveVisual`, `RemoveAllVisuals`,
  `SetRoot` and object release take the device critical section. Window
  calls on the target's children are made from the committing thread, which
  Chromium keeps on its GPU thread.
* Not implemented: transforms other than translations (a FIXME once on
  `SetTransform`), `IDCompositionRectangleClip`/effect/animation objects
  (`E_NOTIMPL`), `IDCompositionDevice` (v1) and `CreateSurfaceFromHwnd`.

## Test Focus
* Patch application against Valve Wine `36078f5` plus GE-Proton11-5
  wine-wayland hotfixes (`patch -p1 --fuzz=0` in order).
* Build artifacts for `dcomp.dll` and `dxgi.dll` are overlaid into the Proton
  tool for both x86_64 and i386 where GE-Proton ships them.
* Runtime logs (`WINEDEBUG=+dcomp`) should show the launcher using
  D3D11/DXGI/DComp, `CreateSwapChainForComposition`, and
  `D3D11SwapChain::SetCompositionTarget` binding to the launcher window.
* Battle.net: the login window renders, no black or white host rectangles,
  no flicker when Chromium reorders visuals, popups/tooltips appear and
  disappear cleanly, and closing the login window does not leave a zombie
  target presenting to a dead HWND.
