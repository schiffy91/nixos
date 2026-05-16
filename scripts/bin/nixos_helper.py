#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p "python3.withPackages (ps: [ ps.pyqt6 ])"
import os, sys, json, time, signal, atexit, ctypes, subprocess
from pathlib import Path
from PyQt6.QtWidgets import QApplication, QSystemTrayIcon, QMenu
from PyQt6.QtGui import QIcon, QAction, QActionGroup, QCursor, QGuiApplication
from PyQt6.QtCore import QObject, QTimer, QPoint, pyqtSlot

PID_FILE = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "nixos-helper.pid"
DEBUG_DUMP = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "nixos-helper-kscreen.json"
INHIBIT_WHO = "nixos-helper"
PR_SET_PDEATHSIG = 1
libc = ctypes.CDLL("libc.so.6", use_errno=True)

DISPLAY_FRIENDLY = { # connector name → friendly label (edid is empty on this hardware)
    "DP-1": "ProXDR",
    "DP-3": "Streaming",
    "HDMI-A-2": "S89C",
}

KSCREEN_TYPE = { # libkscreen Output::Type enum
    1: "VGA", 2: "DVI", 3: "DVI-I", 4: "DVI-A", 5: "DVI-D",
    6: "HDMI", 7: "Panel", 8: "TV", 14: "DisplayPort", 15: "Unknown",
}

MENU_STYLE = """
QMenu { padding: 4px; }
QMenu::item { padding: 6px 36px 6px 20px; }
QMenu::separator { height: 1px; margin: 4px 8px; }
QMenu::right-arrow { width: 10px; height: 10px; margin-right: 8px; }
QMenu::indicator { width: 14px; height: 14px; margin-left: 4px; }
"""

def log(*a): print("[nixos-helper]", *a, file=sys.stderr, flush=True)
def die_with_parent(): libc.prctl(PR_SET_PDEATHSIG, signal.SIGTERM, 0, 0, 0)

def kill_existing():
    if PID_FILE.exists():
        try:
            old = int(PID_FILE.read_text().strip())
            if old != os.getpid():
                os.kill(old, signal.SIGTERM)
                for _ in range(40):
                    try: os.kill(old, 0); time.sleep(0.05)
                    except ProcessLookupError: break
        except (ValueError, ProcessLookupError, PermissionError):
            pass
    PID_FILE.write_text(str(os.getpid()))
    atexit.register(cleanup_pid)

def cleanup_pid():
    try:
        if PID_FILE.exists() and PID_FILE.read_text().strip() == str(os.getpid()):
            PID_FILE.unlink()
    except Exception:
        pass

class Helper(QObject):
    def __init__(self, app):
        super().__init__()
        self.app = app
        self.inhibit_proc = None
        icon = QIcon.fromTheme("nix-snowflake")
        if icon.isNull(): icon = QIcon.fromTheme("system-help")
        self.tray = QSystemTrayIcon(icon)
        self.tray.setToolTip("NixOS Helper")
        self.menu = QMenu()
        self.menu.setStyleSheet(MENU_STYLE)
        self.tray.setContextMenu(self.menu)
        self.tray.activated.connect(self.on_activated)
        self.menu.aboutToShow.connect(self.rebuild_menu)
        self.rebuild_menu()
        self.tray.show()

    def on_activated(self, reason):
        log(f"activated reason={reason!r}")
        if reason != QSystemTrayIcon.ActivationReason.Unknown: # show on any real click
            self.menu.popup(self.popup_pos())

    def popup_pos(self):
        geo = self.tray.geometry()
        if geo.isValid() and geo.x() >= 0 and geo.y() >= 0: # x11 path
            return geo.bottomLeft()
        screen = QGuiApplication.primaryScreen()
        if screen: # wayland: tray geo is invalid; anchor near bottom-right of screen
            r = screen.availableGeometry()
            return QPoint(r.right() - 280, r.bottom() - 200)
        return QCursor.pos()

    def add_header(self, menu, text):
        a = QAction(text, menu); a.setEnabled(False)
        f = a.font(); f.setBold(True); a.setFont(f)
        menu.addAction(a); return a

    def rebuild_menu(self):
        self.menu.clear()
        caffeine = QAction("Caffeine", self.menu)
        caffeine.setCheckable(True); caffeine.setChecked(self.is_inhibiting())
        caffeine.toggled.connect(self.set_sleep_inhibit)
        self.menu.addAction(caffeine)
        outputs = self.get_outputs()
        displays = self.menu.addMenu("Displays")
        displays.setStyleSheet(MENU_STYLE)
        displays.addAction("Turn Off", lambda: self.kscreen("--dpms", "off"))
        if not outputs:
            none_action = displays.addAction("(none detected)")
            none_action.setEnabled(False)
        else:
            displays.addSeparator()
            self.add_header(displays, "Enabled")
            for o in outputs:
                a = QAction(self.friendly(o), displays)
                a.setCheckable(True); a.setChecked(bool(o.get("enabled")))
                n = o["name"]
                a.toggled.connect(lambda on, n=n: self.kscreen(f"output.{n}.{'enable' if on else 'disable'}"))
                displays.addAction(a)
            displays.addSeparator()
            self.add_header(displays, "Primary")
            primary_group = QActionGroup(displays); primary_group.setExclusive(True)
            for o in outputs:
                a = QAction(self.friendly(o), primary_group)
                a.setCheckable(True); a.setChecked(self.is_primary(o))
                n = o["name"]
                a.triggered.connect(lambda _checked, n=n: self.kscreen(f"output.{n}.priority.1"))
                displays.addAction(a)
        self.menu.addSeparator()
        self.menu.addAction("Update", self.do_update)
        self.menu.addAction("Upgrade", self.do_upgrade)
        self.menu.addSeparator()
        self.menu.addAction("Quit", self.quit)

    def get_outputs(self):
        try:
            r = subprocess.run(["kscreen-doctor", "-j"], capture_output=True, text=True, check=True, timeout=3)
            try: DEBUG_DUMP.write_text(r.stdout) # so we can inspect field names
            except Exception: pass
            return json.loads(r.stdout).get("outputs", [])
        except Exception as e:
            log(f"get_outputs failed: {e}")
            return []

    def is_primary(self, o):
        return o.get("priority") == 1 or bool(o.get("primary"))

    def friendly(self, o):
        name = o.get("name", "?")
        if name in DISPLAY_FRIENDLY:
            return f"{DISPLAY_FRIENDLY[name]} ({name})"
        return f"{KSCREEN_TYPE.get(o.get('type'), 'Display')} ({name})"

    def kscreen(self, *args):
        log("kscreen-doctor", *args)
        subprocess.Popen(["kscreen-doctor", *args])

    def is_inhibiting(self):
        return self.inhibit_proc is not None and self.inhibit_proc.poll() is None

    @pyqtSlot(bool)
    def set_sleep_inhibit(self, on):
        if on and not self.is_inhibiting():
            self.inhibit_proc = subprocess.Popen(
                ["systemd-inhibit", "--what=idle", f"--who={INHIBIT_WHO}",
                 "--why=User disabled sleep", "sleep", "infinity"],
                preexec_fn=die_with_parent,
            )
        elif not on and self.is_inhibiting():
            self.inhibit_proc.terminate()
            self.inhibit_proc = None

    def do_update(self): self.terminal("sudo nixos update")
    def do_upgrade(self): self.terminal("sudo nixos upgrade")

    def terminal(self, cmd):
        subprocess.Popen(["konsole", "-e", "bash", "-c",
                          f'{cmd}; echo; read -rp "Press enter to close..."'])

    def quit(self):
        if self.is_inhibiting():
            try: self.inhibit_proc.terminate()
            except Exception: pass
        self.app.quit()

def main():
    kill_existing()
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False)
    helper = Helper(app)
    signal.signal(signal.SIGTERM, lambda *a: helper.quit())
    signal.signal(signal.SIGINT, lambda *a: helper.quit())
    nudge = QTimer(); nudge.timeout.connect(lambda: None); nudge.start(200)
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
