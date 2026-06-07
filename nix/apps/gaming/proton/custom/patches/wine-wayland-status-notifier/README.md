# StatusNotifierItem systray for winewayland.drv

## Symptom
Wine apps with system tray icons (Battle.net's "minimize to tray", Steam,
Discord) lose their icon entirely on Wayland.  No icon appears in KDE
Plasma 6's systray, GNOME's AppIndicator extension, waybar, or any other
Wayland shell.

## Root cause
Wayland has no equivalent of the X11 `_NET_SYSTEM_TRAY` XEmbed protocol
that `winex11.drv` uses for tray icons. `winewayland.drv` does not publish
Wine notify icons to a Wayland-native tray protocol, so icons added through
`Shell_NotifyIconW` never register with the desktop shell.

## Fix
Implement the existing `pSystrayDock*` driver hooks for
`winewayland.drv` against the StatusNotifierItem D-Bus spec - KDE's
freedesktop-pending standard adopted by Plasma 6, GNOME (via the
AppIndicator extension), waybar, and most other Wayland shells.

The driver keeps one shared D-Bus connection and Wine-created dispatch
thread, exports one SNI object per docked Wine tray icon, and forwards SNI
activation methods by queueing a private Wayland driver window message.
The actual Win32 tray callback then runs on Wine's window-message path
instead of a foreign pthread.

Keeping the SNI object in explorer's tray host is important: apps such as
Battle.net register the icon from a short-lived launcher process, then
hand off to a long-lived UI process. Publishing directly from the
`Shell_NotifyIconW` caller drops the D-Bus name as soon as that launcher
exits. Explorer already owns Wine's tray lifetime, so the Wayland bridge
publishes from there.

The active series passes a small driver-facing icon snapshot through
`pSystrayDockInsert`. The snapshot carries only the HICON, owner/window HWNDs,
callback message, notification version, id, and tooltip text.

Explorer also forwards a fresh snapshot for visible docked icons on
`NIM_MODIFY` and `NIM_SETVERSION`. Without that update path, Plasma can keep a
stale tooltip/title/icon and Battle.net can keep the wrong notify-icon callback
version after its launcher hands off to the long-lived UI process.

## Affected Upstream
Current patches:
`dlls/winewayland.drv/{Makefile.in,waylanddrv.h,wayland_systray.c,waylanddrv_main.c,window.c}`,
`programs/explorer/systray.c`, `include/ntuser.h`, `include/wine/gdi_driver.h`,
`dlls/win32u/driver.c`, and the X11 driver hook signature.
