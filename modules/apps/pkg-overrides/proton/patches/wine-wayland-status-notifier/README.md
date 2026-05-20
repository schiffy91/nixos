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

The active Battle.net build rebuilds the matching Wine PE and Unix-side
modules touched by this series, including `win32u`, so generated Unix
call tables stay ABI-matched. For now the Wayland driver reads the
existing explorer tray icon payload and exports it as a
StatusNotifierItem service. A future upstream pass should replace that
private layout dependency with a driver-facing snapshot type before this
is submitted outside the local Proton package.

## Affected upstream
`dlls/winewayland.drv/{Makefile.in, waylanddrv.h, waylanddrv_main.c,
wayland_systray.c (new)}`.
