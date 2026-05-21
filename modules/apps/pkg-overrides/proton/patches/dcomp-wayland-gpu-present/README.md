# DComp/DXGI accelerated Wayland presentation

## Status
Active local series. This folder keeps the Wine half of the
DirectComposition work small: implement the DComp objects Battle.net
uses, then bind composition swap chains to their target HWND. DXVK owns
the actual Wayland WSI presentation path.

## Organization
The series is split by upstream ownership:

1. `dcomp`: implement the minimal D3D11-backed desktop device, target,
   visual, and surface objects needed to bind composition content.
2. `dxgi`: let `CreateSwapChainForComposition` create a hidden owner
   swap chain for Wine's builtin DXGI path.

The active path intentionally does not use `--disable-gpu-compositing`,
`--use-angle=desktop`, or a CPU `wl_shm` readback. Those remain useful
diagnostics, not acceptable defaults.

## Review Notes
The earlier dma-buf/subsurface prototype was removed from the active
series because it copied between DXVK-owned images and compositor-owned
images outside DXVK's normal presenter. The current design is simpler:
Wine's `dcomp.dll` records the visual tree and asks DXVK to retarget the
composition swap chain to the real DComp target window. That keeps Win32
COM behavior in Wine and native presentation in DXVK's existing WSI code.

## Test Focus
* Patch application against Valve Wine `1729f00` plus GE-Proton10-34
  wine-wayland hotfixes.
* Build artifacts for `dcomp.dll`, `dxgi.dll`, and `win32u` are overlaid
  into the Proton tool for both x86_64 and i386 where GE-Proton ships them.
* Runtime logs should show the launcher using D3D11/DXGI/DComp, DXVK's
  Battle.net profile enabling dummy composition swap chains, and
  `D3D11SwapChain::SetCompositionTarget` binding to the launcher window.
