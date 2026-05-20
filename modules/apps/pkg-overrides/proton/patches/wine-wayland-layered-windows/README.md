# Layered windows render blank under winewayland.drv

## Symptom
Win32 layered toplevels (tooltips, splash screens, Battle.net's login
splash) display as transparent / empty rectangles.  Bringing the layered
window back into focus or moving the parent does not refresh it.

## Root cause
`WAYLAND_UpdateLayeredWindow` is missing from the driver's `user_driver_funcs`
table, so `UpdateLayeredWindow` / `SetLayeredWindowAttributes` silently no-op
for any wayland window.

## Fix
Hook `pUpdateLayeredWindow` to `ensure_window_surface_contents`.

## Affected upstream
`dlls/winewayland.drv/{waylanddrv.h, waylanddrv_main.c, window.c}`.
