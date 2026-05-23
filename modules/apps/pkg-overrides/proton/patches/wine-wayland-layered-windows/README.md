# Layered windows render blank under winewayland.drv

## Symptom
Win32 layered toplevels (tooltips, splash screens, Battle.net's login
splash) display as transparent / empty rectangles. Bringing the layered
window back into focus or moving the parent does not refresh it.

Some layered menus render their background and icons but lose text. Those
windows use constant layered-window alpha, not per-pixel alpha, so the
Wayland ARGB SHM upload needs to set the alpha channel from the window
surface metadata instead of trusting whatever GDI left in the DIB alpha
bytes.

Other Chromium/CEF popups and dialogs use 32-bit per-pixel layered surfaces,
but still draw some text/control pixels with RGB data and a zero alpha byte.
Those non-empty pixels need to be promoted to opaque during the upload while
leaving transparent black pixels untouched. Some popup snapshots have no
non-zero alpha at all; in that case the dirty region is treated as opaque so
black text and controls are not dropped.

## Root cause
`WAYLAND_UpdateLayeredWindow` is missing from the driver's `user_driver_funcs`
table, so `UpdateLayeredWindow` / `SetLayeredWindowAttributes` silently no-op
for any wayland window.

## Fix
Hook `pUpdateLayeredWindow` to `ensure_window_surface_contents`, and set
constant alpha bits when copying non-per-pixel-alpha layered surface content
to Wayland ARGB SHM buffers. For per-pixel layered surfaces, preserve the
application-provided alpha channel except for non-black RGB pixels whose alpha
byte is zero, unless the updated region has no non-zero alpha, in which case
the region is uploaded as opaque.

## Affected upstream
`dlls/winewayland.drv/{waylanddrv.h, waylanddrv_main.c, window.c,
window_surface.c}`.
