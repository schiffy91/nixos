# Layered windows render blank under winewayland.drv

## Symptom
Layered toplevels that use constant alpha or a color key
(`SetLayeredWindowAttributes`) come out as transparent rectangles or lose
their text: tooltips, splash screens and menus draw their pixels, but the
alpha byte GDI leaves in the DIB is zero, so the compositor shows nothing.

## Root cause
`wayland_window_surface_flush()` copies the window surface DIB into an
ARGB8888 `wl_shm` buffer verbatim. That is right for per-pixel alpha
surfaces (`UpdateLayeredWindow` with `ULW_ALPHA`), whose alpha channel the
application filled in, but wrong for constant-alpha layered surfaces, where
GDI never writes the alpha byte. winex11.drv derives the alpha from the
window surface metadata (`alpha_bits` / `alpha_mask`) in that case;
winewayland.drv did not.

GE-Proton11 ships the `pUpdateLayeredWindow` hook and the
`wp_alpha_modifier_v1` support (wine-wayland 0013/0014), so only the SHM
upload fix is carried here.

## Fix
Derive the upload alpha from the window surface metadata:

* Surfaces without a per-pixel alpha mask are uploaded opaque.
* If the window has a constant `LWA_ALPHA` value and the compositor does
  not offer `wp_alpha_modifier_v1`, that value is baked into the pixels,
  premultiplied as ARGB8888 `wl_shm` buffers require. When the compositor
  does offer the protocol the driver already applies the alpha through the
  surface multiplier, and baking it as well would apply it twice.
* Surfaces with a per-pixel alpha mask keep the application-provided alpha.
  Fully transparent pixels are already zeroed by the shape pass win32u runs
  on those surfaces, so no per-pixel fixup is needed or possible here.

## Affected upstream
`dlls/winewayland.drv/window_surface.c`.
