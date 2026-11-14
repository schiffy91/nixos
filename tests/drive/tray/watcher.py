#!/usr/bin/env python3
"""The part of a desktop's panel a tray item needs: somewhere to register."""
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib


class Watcher(dbus.service.Object):
    NAME = "org.kde.StatusNotifierWatcher"

    @dbus.service.method(NAME, in_signature="s", sender_keyword="sender")
    def RegisterStatusNotifierItem(self, item, sender=None):
        with open("/tmp/tray-items", "a") as items:
            items.write(f"{item}\n")

    @classmethod
    def main(cls):
        DBusGMainLoop(set_as_default=True)
        bus = dbus.SessionBus()
        name = dbus.service.BusName(cls.NAME, bus)
        cls(bus, "/StatusNotifierWatcher", name)
        GLib.MainLoop().run()


if __name__ == "__main__":
    Watcher.main()
