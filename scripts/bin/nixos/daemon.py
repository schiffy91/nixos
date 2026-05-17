#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p "python3.withPackages (ps: [ ps.pyqt6 ])"
import atexit, ctypes, json, os, signal, subprocess, sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Utils
from PyQt6.QtWidgets import (
    QApplication, QSystemTrayIcon, QMenu, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QCheckBox, QPushButton, QRadioButton, QButtonGroup, QFrame, QScrollArea,
)
from PyQt6.QtGui import QIcon, QAction, QActionGroup, QCursor, QGuiApplication
from PyQt6.QtCore import QObject, QTimer, QPoint, Qt

PID_FILE = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "nixos-helper.pid"
CLI_BINARY = os.environ.get("NIXOS_CLI", "nixos")
PR_SET_PDEATHSIG = 1
libc = ctypes.CDLL("libc.so.6", use_errno=True)

MENU_STYLESHEET = """
QMenu { padding: 4px; }
QMenu::item { padding: 6px 36px 6px 20px; }
QMenu::separator { height: 1px; margin: 4px 8px; }
QMenu::right-arrow { width: 10px; height: 10px; margin-right: 8px; }
QMenu::indicator { width: 14px; height: 14px; margin-left: 4px; }
"""


def die_with_parent(): libc.prctl(PR_SET_PDEATHSIG, signal.SIGTERM, 0, 0, 0)


def cli(*args, capture=False):
    Utils.log(f"cli {' '.join(args)}")
    if capture:
        result = subprocess.run([CLI_BINARY, *args], capture_output=True, text=True, check=False)
        return result.stdout, result.returncode
    return subprocess.Popen([CLI_BINARY, *args])


def cli_json(*args):
    stdout, _ = cli(*args, capture=True)
    try: return json.loads(stdout)
    except json.JSONDecodeError: return []


def cli_bool(*args):
    _, returncode = cli(*args, capture=True)
    return returncode == 0


def kill_existing():
    if PID_FILE.exists():
        try:
            existing_pid = int(PID_FILE.read_text().strip())
            if existing_pid != os.getpid():
                os.kill(existing_pid, signal.SIGTERM)
                for _ in range(40):
                    try: os.kill(existing_pid, 0); time.sleep(0.05)
                    except ProcessLookupError: break
        except (ValueError, ProcessLookupError, PermissionError): pass
    PID_FILE.write_text(str(os.getpid()))
    atexit.register(cleanup_pid)


def cleanup_pid():
    try:
        if PID_FILE.exists() and PID_FILE.read_text().strip() == str(os.getpid()):
            PID_FILE.unlink()
    except Exception: pass


class HelperWindow(QWidget):
    def __init__(self, helper):
        super().__init__()
        self.helper = helper
        self.setWindowTitle("NixOS Helper")
        self.setWindowFlags(self.windowFlags() | Qt.WindowType.Tool)
        self.setMinimumSize(360, 480)
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setFrameShape(QFrame.Shape.NoFrame)
        inner_widget = QWidget()
        self.section_layout = QVBoxLayout(inner_widget)
        self.section_layout.setSpacing(12)
        scroll_area.setWidget(inner_widget)
        root_layout = QVBoxLayout(self)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.addWidget(scroll_area)

    def header(self, text):
        label = QLabel(text)
        font = label.font()
        font.setBold(True)
        font.setPointSizeF(font.pointSizeF() + 1)
        label.setFont(font)
        return label

    def rule(self):
        frame = QFrame()
        frame.setFrameShape(QFrame.Shape.HLine)
        frame.setFrameShadow(QFrame.Shadow.Sunken)
        return frame

    def refresh(self):
        while self.section_layout.count():
            item = self.section_layout.takeAt(0)
            widget = item.widget()
            if widget: widget.deleteLater()
        # Caffeine
        self.section_layout.addWidget(self.header("Caffeine"))
        caffeine_checkbox = QCheckBox("Inhibit sleep")
        caffeine_checkbox.setChecked(self.helper.caffeine_on())
        caffeine_checkbox.toggled.connect(self.helper.toggle_caffeine)
        self.section_layout.addWidget(caffeine_checkbox)
        self.section_layout.addWidget(self.rule())
        # Displays
        self.section_layout.addWidget(self.header("Displays"))
        dpms_button = QPushButton("Turn All Off")
        dpms_button.clicked.connect(lambda: cli("displays", "dpms", "off"))
        self.section_layout.addWidget(dpms_button)
        for output in cli_json("displays", "list"):
            row = QHBoxLayout()
            enable_checkbox = QCheckBox(f"{output.get('label', output['name'])} ({output['name']})")
            enable_checkbox.setChecked(bool(output.get("enabled")))
            enable_checkbox.toggled.connect(
                lambda enabled, name=output["name"]: cli("displays", "enable" if enabled else "disable", name))
            row.addWidget(enable_checkbox, 1)
            if output.get("enabled"):
                primary_button = QPushButton("Set Primary")
                primary_button.setEnabled(output.get("priority") != 1)
                primary_button.clicked.connect(lambda _checked=False, name=output["name"]: cli("displays", "primary", name))
                row.addWidget(primary_button)
            self.section_layout.addLayout(row)
        self.section_layout.addWidget(self.rule())
        # Audio
        self.section_layout.addWidget(self.header("Audio"))
        audio_group = QButtonGroup(self)
        audio_group.setExclusive(True)
        for sink in cli_json("audio", "list"):
            radio = QRadioButton(sink.get("label") or sink.get("description") or sink["name"])
            radio.setChecked(bool(sink.get("default")))
            radio.toggled.connect(lambda selected, name=sink["name"]: selected and cli("audio", "set", name))
            audio_group.addButton(radio)
            self.section_layout.addWidget(radio)
        self.section_layout.addWidget(self.rule())
        # System
        self.section_layout.addWidget(self.header("System"))
        for label, operation in (("Update", "update"), ("Upgrade", "upgrade")):
            button = QPushButton(label)
            button.clicked.connect(lambda _checked=False, op=operation: cli("system", op))
            self.section_layout.addWidget(button)
        self.section_layout.addStretch(1)

    def show_centered(self):
        self.refresh()
        primary_screen = QGuiApplication.primaryScreen()
        if primary_screen:
            available = primary_screen.availableGeometry()
            self.move(available.center() - self.rect().center())
        self.show()
        self.raise_()
        self.activateWindow()


class Helper(QObject):
    def __init__(self, app):
        super().__init__()
        self.app = app
        icon = QIcon.fromTheme("nix-snowflake")
        if icon.isNull(): icon = QIcon.fromTheme("system-help")
        self.tray = QSystemTrayIcon(icon)
        self.tray.setToolTip("NixOS Helper")
        self.menu = QMenu()
        self.menu.setStyleSheet(MENU_STYLESHEET)
        self.tray.setContextMenu(self.menu)
        self.tray.activated.connect(self.on_activated)
        self.menu.aboutToShow.connect(self.rebuild_menu)
        self.window = HelperWindow(self)
        self.rebuild_menu()
        self.tray.show()

    def caffeine_on(self): return cli_bool("caffeine", "status")

    def toggle_caffeine(self, enabled): cli("caffeine", "enable" if enabled else "disable")

    def on_activated(self, reason):
        Utils.log(f"tray activated reason={reason!r}")
        if reason == QSystemTrayIcon.ActivationReason.Trigger:
            self.window.show_centered()
        elif reason != QSystemTrayIcon.ActivationReason.Unknown:
            self.menu.popup(self.popup_position())

    def popup_position(self):
        geometry = self.tray.geometry()
        if geometry.isValid() and geometry.x() >= 0 and geometry.y() >= 0:
            return geometry.bottomLeft()
        primary_screen = QGuiApplication.primaryScreen()
        if primary_screen:
            available = primary_screen.availableGeometry()
            return QPoint(available.right() - 280, available.bottom() - 200)
        return QCursor.pos()

    def add_header(self, menu, text):
        action = QAction(text, menu)
        action.setEnabled(False)
        font = action.font()
        font.setBold(True)
        action.setFont(font)
        menu.addAction(action)
        return action

    def rebuild_menu(self):
        self.menu.clear()
        caffeine_action = QAction("Caffeine", self.menu)
        caffeine_action.setCheckable(True)
        caffeine_action.setChecked(self.caffeine_on())
        caffeine_action.toggled.connect(self.toggle_caffeine)
        self.menu.addAction(caffeine_action)

        displays_menu = self.menu.addMenu("Displays")
        displays_menu.setStyleSheet(MENU_STYLESHEET)
        displays_menu.addAction("Turn Off", lambda: cli("displays", "dpms", "off"))
        outputs = cli_json("displays", "list")
        if not outputs:
            none_action = displays_menu.addAction("(none detected)")
            none_action.setEnabled(False)
        else:
            displays_menu.addSeparator()
            self.add_header(displays_menu, "Enabled")
            for output in outputs:
                action = QAction(f"{output.get('label', output['name'])} ({output['name']})", displays_menu)
                action.setCheckable(True)
                action.setChecked(bool(output.get("enabled")))
                action.toggled.connect(
                    lambda enabled, name=output["name"]: cli("displays", "enable" if enabled else "disable", name))
                displays_menu.addAction(action)
            displays_menu.addSeparator()
            self.add_header(displays_menu, "Primary")
            primary_group = QActionGroup(displays_menu)
            primary_group.setExclusive(True)
            for output in outputs:
                if not output.get("enabled"): continue
                action = QAction(f"{output.get('label', output['name'])} ({output['name']})", primary_group)
                action.setCheckable(True)
                action.setChecked(output.get("priority") == 1)
                action.triggered.connect(lambda _checked, name=output["name"]: cli("displays", "primary", name))
                displays_menu.addAction(action)

        self.menu.addSeparator()
        audio_menu = self.menu.addMenu("Audio")
        audio_menu.setStyleSheet(MENU_STYLESHEET)
        audio_group = QActionGroup(audio_menu)
        audio_group.setExclusive(True)
        for sink in cli_json("audio", "list"):
            action = QAction(sink.get("label") or sink.get("description") or sink["name"], audio_group)
            action.setCheckable(True)
            action.setChecked(bool(sink.get("default")))
            action.triggered.connect(lambda _checked, name=sink["name"]: cli("audio", "set", name))
            audio_menu.addAction(action)

        self.menu.addSeparator()
        self.menu.addAction("Open Window", self.window.show_centered)
        self.menu.addSeparator()
        self.menu.addAction("Update", lambda: cli("system", "update"))
        self.menu.addAction("Upgrade", lambda: cli("system", "upgrade"))
        self.menu.addSeparator()
        self.menu.addAction("Quit", self.quit)

    def quit(self):
        cli("caffeine", "disable")
        self.app.quit()


def main():
    kill_existing()
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False)
    helper = Helper(app)
    signal.signal(signal.SIGTERM, lambda *args: helper.quit())
    signal.signal(signal.SIGINT, lambda *args: helper.quit())
    keepalive_timer = QTimer()
    keepalive_timer.timeout.connect(lambda: None)
    keepalive_timer.start(200)
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
