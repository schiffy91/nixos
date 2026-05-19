# StatusNotifierItem systray for winewayland.drv

## Symptom
Wine apps with system tray icons (Battle.net's "minimize to tray", Steam,
Discord) lose their icon entirely on Wayland.  No icon appears in KDE
Plasma 6's systray, GNOME's AppIndicator extension, waybar, or any other
Wayland shell.

## Root cause
Wayland has no equivalent of the X11 `_NET_SYSTEM_TRAY` XEmbed protocol
that `winex11.drv` uses for tray icons.  `winewayland.drv` does not
implement the `pSystrayDock*` driver hooks at all, so the icon never
registers anywhere.

## Fix
Implement `pSystrayDock*` against the StatusNotifierItem D-Bus spec —
KDE's freedesktop-pending standard adopted by Plasma 6, GNOME (via the
AppIndicator extension), waybar, and most other Wayland shells.  Each
icon registers an SNI service via `libdbus`, exports its pixmap, and
translates SNI `Activate` / `SecondaryActivate` signals into
`WM_LBUTTONUP` / `WM_RBUTTONUP` on the wine systray owner window.

Implementation lives in a new `wayland_systray.c` to keep it isolated
and reusable.

## Affected upstream
`dlls/winewayland.drv/{Makefile.in, waylanddrv.h, waylanddrv_main.c,
wayland_systray.c (new)}`.
