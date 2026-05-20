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
Implement `pSystrayDock*` against the StatusNotifierItem D-Bus spec -
KDE's freedesktop-pending standard adopted by Plasma 6, GNOME (via the
AppIndicator extension), waybar, and most other Wayland shells.

The active Battle.net build keeps GE-Proton's stock `win32u` and
`explorer` binaries, because live tests showed rebuilt versions
destabilize libcef before the launcher settles.  For now the Wayland
driver reads the existing explorer tray icon payload and exports it as a
StatusNotifierItem service.  A future upstream pass should replace that
private layout dependency with a driver-facing snapshot once the full
`win32u`/`explorer` stack is validated.

## Affected upstream
`dlls/winewayland.drv/{Makefile.in, waylanddrv.h, waylanddrv_main.c,
wayland_systray.c (new)}`.
