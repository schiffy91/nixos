# nixosctl tray

`import tray.Tray;` provides `SystemTray`, a native status item backed by a
private Linux provider. The stdlib's `Library.Tray` ships a macOS provider only,
so this package follows the same layout the stdlib README prescribes: a facade
module at the package root, a platform provider in a platform subfolder, and a
`btrc.toml` whose `[[package.providers]]` entry selects it for the compilation
target. Consumers never name the provider.

```btrc
SystemTray("NixOS").icon("/path/icon.png")
	.item("Open /etc/nixos", "xdg-open /etc/nixos")
	.checkItem("Caffeine", "nixosctl caffeine toggle", "nixosctl caffeine status")
	.item("Quit", TraySignal.quit()).run();
```

## Linux provider

`Linux/TrayProvider.btrc` speaks the KDE StatusNotifierItem spec
(`org.kde.StatusNotifierItem`) plus Canonical's `com.canonical.dbusmenu` over
the session bus, which is what Plasma, GNOME's AppIndicator extension and the
wlroots bars consume. It binds libdbus-1 through a typed native import
(`[[native.bindings]]` on `Linux/DBus.h`); `Linux/DBus.h` only adapts the parts
of libdbus the importer cannot lower faithfully: `DBusError` (bitfields) stays
C-side behind three call adapters, and the by-address basic marshalling
(`dbus_message_iter_append_basic` / `get_basic`) becomes by-value helpers.
`[[native.pkg-config]]` names `dbus-1` for the transpile-time header read and
the link.

The provider owns a private bus connection and pumps it by pull: `pump()` reads
the socket, pops every queued method call and answers it itself, so no native
callback or object-path vtable is involved. `run()` blocks in `pump(-1)` until a
`TraySignal.quit()` item is activated or the bus disconnects. A menu activation
is recorded during `Event` and executed through `UnixShell` only after its reply
has left, so the host never waits on the command. Check items are probed when
the menu is realized, on every `AboutToShow`, and after each activation; a
changed mark bumps the dbusmenu revision and emits `LayoutUpdated`.

`show()` returns false, with `SystemTray.failure` set, when there is no session
bus or no `StatusNotifierWatcher` (a headless session); protocol failures throw.
Every unhandled method call gets an `UnknownMethod` error reply rather than a
silent timeout.

## Building

The root manifest `btrc/btrc.toml` names the `nixosctl` application and depends
on this package by path, which is what lets the typed records (`DBusMessageIter`)
lower for entry points outside this folder. `make build` transpiles and links
`build/nixosctl-tray`; `nix/apps/utils/nixosctl/nixosctl-tray.nix` builds the
same entry with `pkg-config` and `dbus` in scope.
