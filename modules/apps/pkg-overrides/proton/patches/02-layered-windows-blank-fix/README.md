# Layered windows render blank under winewayland.drv

## Symptom
Win32 layered toplevels (tooltips, splash screens, Battle.net's login
splash) display as transparent / empty rectangles.  Bringing the layered
window back into focus or moving the parent does not refresh it.

## Root cause
Two independent bugs in the same code path:

1. `WAYLAND_UpdateLayeredWindow` is missing from the driver's
   `user_driver_funcs` table, so `UpdateLayeredWindow` /
   `SetLayeredWindowAttributes` silently no-op for any wayland window.
2. After `xdg_surface` reconfigure (output add/remove, DPI change), the
   compositor drops the previously attached buffer but the window proc
   is not woken to redraw, so the next commit has no buffer.

## Fix
1. Hook `pUpdateLayeredWindow` to `ensure_window_surface_contents`.
2. Trigger `ensure_window_surface_contents` from the
   `WM_WAYLAND_CONFIGURE` handler so reconfigure events repaint.

## Affected upstream
`dlls/winewayland.drv/{waylanddrv.h, waylanddrv_main.c, window.c}`.
