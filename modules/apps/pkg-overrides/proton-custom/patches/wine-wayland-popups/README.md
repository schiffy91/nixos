# Wayland transient popups

## Symptom
Winewayland can map owned popup windows as independent toplevels.  On Plasma,
that lets tray menus and nested context menus fall below shell surfaces such as
the bottom panel, even when the app created the popup from a foreground window.

## Fix
Map owner-relative popup windows to `xdg_popup` when the owner already has an
`xdg_surface`.  The compositor can then stack and constrain menus as popups
instead of treating them as normal application windows.

The same patch requests activation of the owner toplevel before delivering SNI
context-menu callbacks, so applications have a foreground parent when they build
their menu.
