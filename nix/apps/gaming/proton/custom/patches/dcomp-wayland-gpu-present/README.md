# DComp/DXGI accelerated Wayland presentation

## Status
Active Wine series. This folder keeps the Wine half of the DirectComposition
work small: implement the DComp objects Battle.net uses, then bind composition
swap chains and GPU-backed DComp surfaces to their target HWND. DXVK owns the
actual Wayland WSI presentation path.

## Organization
The series is split by upstream ownership:

1. `dcomp`: implement the minimal D3D11-backed desktop device, target,
   visual, surface, virtual surface, and scroll behavior needed to bind
   composition content.
2. `dxgi`: let `CreateSwapChainForComposition` create a hidden owner
   swap chain for Wine's builtin DXGI path.

The active path intentionally does not use `--disable-gpu-compositing`,
`--use-angle=desktop`, or a CPU `wl_shm` readback. Those remain useful
diagnostics, not acceptable defaults.

Unplaced first-level swap-chain wrappers bind directly to the target HWND. Those
visuals are often just CEF's identity wrapper around the real page content, and
hosting them in a Win32 child HWND leaves native Wayland with an unmanaged child
surface that can present successfully without becoming visible. Placed, clipped,
nested, and surface-backed visuals still use child host HWNDs so their geometry
and stacking stay independent from the target window.

Composition host geometry and clip regions are cached after each successful
Win32 update. Commits that do not change the visual tree avoid restacking or
resetting child-window regions, which keeps Wayland configure traffic low
during CEF animations and scrolling.

Hosts are explicitly unbound from DXVK before they are destroyed or replaced.
If a visual requires a host for placement, clipping, or surface presentation
but no valid host exists, the content is left unbound instead of being
presented on the root HWND with incorrect geometry.

Composition host HWNDs are created hidden. Surface hosts are shown after Wine
has presented their backing content; swap-chain hosts stay hidden until DXVK
receives the first present for that composition swap chain. This prevents an
empty host window from being mapped during the CEF login-to-launcher handoff.

When a visual is backed by a child host HWND, the target parent is marked with
`WS_CLIPCHILDREN`. The host HWND is our stand-in for a DComp compositor layer;
parent Qt/CEF background repaints must not cover that child after DXVK has
presented Chromium content.

## Review Notes
Wine's `dcomp.dll` records the visual tree and asks DXVK to retarget each
composition swap chain to the real DComp target HWND or to a child host HWND
when placement, clipping, or surface content requires one. That keeps Win32 COM
behavior in Wine and native presentation in DXVK's existing WSI code.

## Test Focus
* Patch application against Valve Wine `36078f5` plus GE-Proton11-5
  wine-wayland hotfixes. `IDCompositionVisual2Vtbl` is initialized
  positionally, so re-check the slot order in `include/dcomp.idl` on every
  base bump — Proton 11 swapped the `Set*`/`Set*Object` and
  `Set*`/`Set*Animation` pairs.
* Build artifacts for `dcomp.dll`, `dxgi.dll`, `explorer.exe`,
  `winewayland.drv`, `winevulkan`, and `win32u` are overlaid into the Proton
  tool for both x86_64 and i386 where GE-Proton ships them.
* Runtime logs should show the launcher using D3D11/DXGI/DComp,
  `CreateSwapChainForComposition`, and `D3D11SwapChain::SetCompositionTarget`
  binding to the launcher window.
* Chromium/CEF shared-image paths should use Wine's D3DKMT shared-resource
  plumbing rather than a DXVK application profile.
