# DComp/DXGI accelerated Wayland presentation

## Status
Active local series. This folder keeps the Wine half of the
DirectComposition work small: implement the DComp objects Battle.net
uses, then bind composition swap chains and GPU-backed DComp surfaces to
their target HWND. DXVK owns the actual Wayland WSI presentation path.

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

Opaque root swap chains stay bound to the target HWND. Opaque child or
placed visuals get host HWNDs, because DComp placement, clipping, and
z-order do not depend on alpha mode.

Composition host geometry and clip regions are cached after each successful
Win32 update. Commits that do not change the visual tree avoid restacking or
resetting child-window regions, which keeps Wayland configure traffic low
during CEF animations and scrolling.

Hosts are explicitly unbound from DXVK before they are destroyed or replaced.
If a visual requires a host for placement, clipping, or surface presentation
but no valid host exists, the content is left unbound instead of being
presented on the root HWND with incorrect geometry.

## Review Notes
The earlier dma-buf/subsurface prototype was removed from the active
series because it copied between DXVK-owned images and compositor-owned
images outside DXVK's normal presenter. The current design is simpler:
Wine's `dcomp.dll` records the visual tree and asks DXVK to retarget each
composition swap chain to the real DComp target HWND or to a child host HWND
when placement, clipping, or surface content requires one. That keeps Win32
COM behavior in Wine and native presentation in DXVK's existing WSI code.

## Test Focus
* Patch application against Valve Wine `1729f00` plus GE-Proton10-34
  wine-wayland hotfixes.
* Build artifacts for `dcomp.dll`, `dxgi.dll`, `explorer.exe`,
  `winewayland.drv`, `winevulkan`, and `win32u` are overlaid into the Proton
  tool for both x86_64 and i386 where GE-Proton ships them.
* Runtime logs should show the launcher using D3D11/DXGI/DComp, DXVK's
  Battle.net profile enabling dummy composition swap chains, and
  `D3D11SwapChain::SetCompositionTarget` binding to the launcher window.
* Chromium/CEF shared-image paths should not rely on Wine's incomplete
  `D3DKMTOpenResource2()` path for Battle.net. The DXVK profile caps the
  reported shared resource tier to zero so CEF stays on ordinary D3D11
  textures while composition swap chains still present through DXVK's Wayland
  WSI path.
