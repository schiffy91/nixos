# winewayland.drv: present DXVK back buffers via wl_subsurface

## Symptom
With patches 04 and 05 applied, `dcomp.dll` snapshots CEF's DXVK back
buffer and sends `WM_WAYLAND_COMPOSE_ATTACH` to the windowing
driver - but `winewayland.drv` has nowhere to put those pixels.  CEF's
output is rendered into a Vulkan image the driver doesn't know how to
present alongside wine's normal CPU bitmap.

## Root cause
`winewayland.drv` presents one `wl_surface` per HWND via `wl_shm` (CPU
bitmap from wine's GDI / win32u path).  CEF's DComp output lives in
DXVK's Vulkan device, in an image format the wayland CPU path can't
consume directly.  The driver needs a second presentation layer for
the GPU-rendered content.

## Fix
Add a Vulkan-to-`wl_shm` copy path:

1. Per HWND that receives `WM_WAYLAND_COMPOSE_ATTACH`, allocate a
   host-visible `VkBuffer` sized for the swap chain back buffer.
2. On each message, `vkCmdCopyImageToBuffer` from the back buffer into
   the host-visible buffer, wait on a fence (capped at 5 ms so we don't
   stall the compositor frame), `memcpy` into a `wl_shm` pool.
3. Attach the resulting `wl_buffer` to a `wl_subsurface` of the
   toplevel ancestor so the compositor composites CEF on top of (or in
   place of) the existing wine bitmap.

Three additional small Wayland integrations are bundled because they
share the same surface plumbing and have no useful meaning on their own:

* **Input region click-through** - `wl_compositor_create_region` with no
  rects so `WS_EX_TRANSPARENT` windows don't intercept clicks intended
  for the parent toplevel.  Required for the launcher's transparent
  overlay regions.
* **HiDPI** - `wp_viewporter` + `wp_fractional_scale_v1`.  The
  subsurface buffer is allocated at full device-pixel resolution and
  the viewport maps it to logical surface coordinates, so HiDPI users
  get crisp 1:1 device pixels instead of bilinear upscale.
* **Cursor shape** - `wp_cursor_shape_v1` `SHAPE_DEFAULT` fallback so
  we do not require `wl_cursor` themes when the compositor advertises
  the cursor-shape protocol.

## Dependencies
Pairs with `04-dcomp-direct-composition`.  Standalone otherwise.

## Affected upstream
`dlls/winewayland.drv/{Makefile.in, waylanddrv.h, wayland_pointer.c,
wayland_compose.c (new), wayland_surface.c, window.c}`.
