# StatusNotifierItem systray for winewayland.drv

## Symptom
Wine apps with system tray icons (Battle.net's "minimize to tray", Steam,
Discord) lose their icon entirely on Wayland. No icon appears in KDE Plasma
6's systray, GNOME's AppIndicator extension, waybar, or any other Wayland
shell.

## Root cause
Wayland has no equivalent of the X11 `_NET_SYSTEM_TRAY` XEmbed protocol
that `winex11.drv` uses for tray icons. `winewayland.drv` does not implement
the `pSystrayDock*` driver hooks, so icons added through `Shell_NotifyIconW`
never reach the desktop shell.

## Fix
Implement the `pSystrayDock*` hooks for `winewayland.drv` against the
StatusNotifierItem D-Bus spec (KDE's freedesktop-pending standard, used by
Plasma 6, GNOME via the AppIndicator extension, waybar and most other
Wayland shells).

### Series
| Patch | Scope |
| --- | --- |
| `0001-win32u-Pass-a-systray-icon-snapshot-to-SystrayDockInsert.patch` | `struct systray_icon_snapshot` in `ntuser.h`; explorer builds it for every dock insert; wow64win converts the 32-bit layout; X11 prototype only. |
| `0002-winewayland.drv-Add-StatusNotifierItem-tray-support.patch` | The driver: `wayland_systray.c`, dispatch thread start, private window message, configure notice. |
| `0003-explorer-Forward-docked-tray-icon-updates.patch` | Explorer repeats the dock insert on `NIM_MODIFY`/`NIM_SETVERSION`; the X11 driver ignores repeats while the icon is embedded and mapped. |

### Design
* **Explorer owns the item lifetime.** Apps such as Battle.net register the
  icon from a short-lived launcher process and then hand off to a long-lived
  UI process. Publishing from the `Shell_NotifyIconW` caller would drop the
  D-Bus name as soon as the launcher exits, so the bridge publishes from
  explorer's tray host: dock insert creates an item (or updates it in place),
  dock remove releases it.
* **One private session bus connection per desktop process.** libdbus is
  `dlopen`ed through `SONAME_LIBDBUS_1` like `mountmgr.sys` does; the build
  only needs `$(DBUS_CFLAGS)`. Without libdbus the driver prints one FIXME and
  explorer falls back to its own tray window.
* **One well-known name per item, one shared object path.** Every docked
  icon claims `org.kde.StatusNotifierItem-<pid>-<serial>`; the name a host
  addresses selects the item. Tradeoff: `NewIcon`/`NewToolTip` signals on the
  shared path refresh every item on hosts that match signals by sender name.
* **Dispatch thread started lazily.** The first dock insert starts a Wine
  thread through a `KeUserDispatchCallback` into the driver's PE side, so
  only the desktop process runs it. The thread loops on
  `dbus_connection_read_write_dispatch` with a 200 ms timeout: the finite
  timeout bounds how long explorer's thread waits for the connection's I/O
  path when it sends signals or blocks on a reply.
* **Win32 callbacks run on Wine's message path.** Activation methods only
  post `WM_WAYLAND_SNI_CALLBACK` (last entry of `enum wayland_window_message`)
  to the icon's adaptor window with a self-contained callback record. The
  window procedure then warps the cursor (only if the reported position lies
  inside Wine's virtual screen), sends the button/`NIN_SELECT`/
  `WM_CONTEXTMENU` notifications, and for context menus waits on the
  `WM_CONTEXTMENU` (or v0 `WM_RBUTTONUP`) with `SMTO_ABORTIFHUNG`, 2 s.
  `Scroll` is answered and dropped: Windows never delivers wheel events to
  tray icons.
* **Pixmaps.** `IconPixmap` and the `ToolTip` pixmap are ARGB32 in network
  byte order. Icons without an alpha channel get their alpha from the 1bpp
  icon mask (same approach as the cursor code in `wayland_surface.c`). The
  pixmap is re-converted on every insert and compared, so `NIM_MODIFY` with a
  reused `HICON` handle value still emits `NewIcon`.

### Environment
`WINE_SNI_ICON_NAME=<theme icon name>` sets the item's `IconName` property.
It is read once when the driver initializes. Hosts that prefer a named icon
(Plasma, when the icon exists in the hicolor theme) show it instead of the
pixmap; the pixmap is still exported for hosts that use it for sizing or as
a fallback. The installed `battlenet` launcher sets it to `battlenet`.

### Watcher and bus lifetime
* **Watcher (re)appears** (`plasmashell --replace`, shell start after Wine):
  the driver watches `NameOwnerChanged` for `org.kde.StatusNotifierWatcher`.
  On a new owner it re-sends `RegisterStatusNotifierItem` for every
  published item with `dbus_connection_send` (never blocking on the dispatch
  thread) and posts `WM_USER + 1` to explorer's tray window, which re-docks
  every icon exactly like `winex11.drv` does when an X11 tray manager
  appears. Items docked while no watcher exists are kept and published, so
  they register as soon as a watcher shows up.
* **Session bus connection lost**: the dispatch thread marks every item
  unpublished, closes the connection, and posts `WM_USER + 1`; explorer's
  re-dock removes the items and inserts them again, which reconnects. Items
  stay in the list until explorer removes them, because explorer asserts that
  a dock remove succeeds for every icon it believes docked.

## Verifying
* `WINEDEBUG=+systray` on explorer shows `registered
  org.kde.StatusNotifierItem-<pid>-<n>` on dock insert and `updated ... changed=`
  on `NIM_MODIFY`/`NIM_SETVERSION`.
* `busctl --user tree org.kde.StatusNotifierItem-<pid>-<n>` lists
  `/StatusNotifierItem`; `busctl --user introspect` on it shows the
  properties (`Title` is the tooltip text).
* Left/right click on the panel icon must deliver the app's callback with
  the notify-icon version semantics it selected (`NIN_SELECT`/`WM_CONTEXTMENU`
  for v4, mouse messages for v0).
* `plasmashell --replace` must bring the icons back without restarting the
  app.

## Affected Upstream
`include/ntuser.h`, `include/wine/gdi_driver.h`, `dlls/win32u/driver.c`,
`dlls/wow64win/user.c`, `dlls/winex11.drv/{window.c,x11drv.h}`,
`programs/explorer/systray.c`, `configure.ac`, and
`dlls/winewayland.drv/{Makefile.in,dllmain.c,unixlib.h,waylanddrv.h,waylanddrv_main.c,window.c,wayland_systray.c}`.
