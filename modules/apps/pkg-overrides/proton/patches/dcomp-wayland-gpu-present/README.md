# DComp/DXGI accelerated Wayland presentation

## Status
Active local series. This folder replaces the previous DComp/DXGI/
subsurface prototypes with one upstream-facing topic: allow D3D11
DirectComposition clients to present through winewayland without
disabling GPU compositing.

## Organization
The series is split by upstream ownership:

1. `dcomp`: implement the minimal D3D11-backed desktop device, target,
   visual, and surface objects needed to bind composition content.
2. `dxgi`: let `CreateSwapChainForComposition` create a hidden owner
   swap chain so DirectComposition clients can stay on the D3D11 path.
3. `winewayland.drv`: add reusable dma-buf buffer import helpers.
4. `winewayland.drv`: copy DXVK's Vulkan image into an exportable image
   and present it through a Wayland subsurface.

The active path intentionally does not use `--disable-gpu-compositing`,
`--use-angle=desktop`, or a CPU `wl_shm` readback. Those remain useful
diagnostics, not acceptable defaults.

## Review Notes
The private bridge between `dcomp.dll` and `winewayland.drv` is currently
a Wine window message carrying Vulkan/DXVK interop handles. That keeps
Win32 COM behavior in `dcomp`, swap-chain creation in `dxgi`, and native
presentation in the driver. Before upstream submission this should be
evaluated against Wine's preferred driver boundary conventions, but it is
deliberately isolated to this topic series.

## Test Focus
* Patch application against Valve Wine `1729f00` plus GE-Proton10-34
  wine-wayland hotfixes.
* Build artifacts for `dcomp.dll`, `dxgi.dll`, `winewayland.drv`, and
  `winewayland.so` are overlaid into the Proton tool.
* Runtime logs should show the launcher using D3D11/DXGI/DComp and the
  Wayland driver creating dma-buf-backed presentation surfaces.
