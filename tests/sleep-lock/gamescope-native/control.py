from typing import TYPE_CHECKING

if TYPE_CHECKING:
    # NixOS supplies these names when executing the test script.
    from nixos_test import machine, start_all  # ty: ignore[unresolved-import]

import json
import shlex
import subprocess
import time
from pathlib import Path
from vncdotool import api


def wait_for_frame_text(client, text, name, timeout=30):
    screenshot = Path(machine.out_dir) / f"{name}.png"
    deadline = time.monotonic() + timeout
    last_text = ""
    while time.monotonic() < deadline:
        client.captureScreen(str(screenshot), incremental=False)
        result = subprocess.run(["tesseract", str(screenshot), "stdout"], capture_output=True, text=True, check=True)
        last_text = result.stdout
        if text in last_text:
            print(f"Verified actual console frame {screenshot}: {last_text}")
            return
        time.sleep(1)
    raise AssertionError(f"Expected {text!r} in actual console frame {screenshot}, got {last_text!r}")

try:
    start_all()
except Exception:
    # QEMU can exit before the driver's serial reader begins.
    if machine.process is not None:
        machine.process.wait(timeout=10)
        if machine.process.stdout is not None:
            print(machine.process.stdout.read().decode(errors="replace"))
    raise
machine.wait_for_unit("multi-user.target")
user_systemctl = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user"
client = None
try:
    machine.wait_until_succeeds(f"{user_systemctl} is-active gamescope-session.service", timeout=60)
    machine.wait_until_succeeds(f"{user_systemctl} is-active steam-launcher.service", timeout=30)
    time.sleep(3)
    journal = machine.succeed("journalctl -b --no-pager _UID=1000")
    selected = [line for line in journal.splitlines() if "selecting physical device" in line]
    assert selected and all("RADV" in line and "Venus" not in line and "llvmpipe" not in line for line in selected), selected
    assert "opening DRM node '/dev/dri/card0'" in journal, journal
    assert machine.succeed(f"{user_systemctl} show gamescope-session.service -p NRestarts --value").strip() == "0"
    machine.wait_for_file("/tmp/gamescope-control-window", timeout=10)
    window = machine.succeed("cat /tmp/gamescope-control-window").strip()
    assert window.isdigit(), window
    environment = dict(line.split("=", 1) for line in machine.succeed("cat /run/user/1000/gamescope-environment").splitlines() if "=" in line)
    xprop = "runuser -u observer -- env DISPLAY=" + shlex.quote(environment["DISPLAY"]) + " xprop"
    print("Test client identity:", machine.succeed(f"{xprop} -id {window} STEAM_GAME WM_NAME"), flush=True)
    focus_query = f"{xprop} -root GAMESCOPECTRL_BASELAYER_APPID GAMESCOPE_FOCUSABLE_APPS GAMESCOPE_FOCUSABLE_WINDOWS GAMESCOPE_FOCUSED_APP GAMESCOPE_FOCUSED_APP_GFX GAMESCOPE_FOCUSED_WINDOW"
    print("Gamescope focus state:", machine.succeed(focus_query), flush=True)
    machine.wait_until_succeeds(f"{xprop} -root GAMESCOPE_FOCUSED_APP_GFX | grep -Fx 'GAMESCOPE_FOCUSED_APP_GFX(CARDINAL) = 769'", timeout=10)
    # QEMU 11.1.2 screendump rejects SCANOUT_TEXTURE/DMABUF with "no surface".
    # Record that exact response, then observe its documented EGL/VNC readback.
    raw = Path(machine.out_dir) / "qmp-diagnostic.ppm"
    print("HMP screendump response:", repr(machine.send_monitor_command(f"screendump {raw}")))
    print("Guest DRM state:", machine.execute("cat /sys/kernel/debug/dri/0/state"))
    socket_path = Path(machine.state_dir) / "gamescope.vnc"
    assert socket_path.is_socket(), socket_path
    client = api.connect(str(socket_path), timeout=10)
    wait_for_frame_text(client, "GAMESCOPE_CONTROL_READY", "gamescope-control-start")
    machine.send_chars("touch /tmp/gamescope-before; echo BEFOREINPUTWORKS\n")
    machine.wait_for_file("/tmp/gamescope-before", timeout=10)
    wait_for_frame_text(client, "BEFOREINPUTWORKS", "gamescope-control-before", timeout=10)
    sessions = json.loads(machine.succeed("loginctl list-sessions --json=short"))
    assert any(session["uid"] == 1000 and session["seat"] == "seat0" for session in sessions), sessions
    assert machine.succeed(f"{user_systemctl} show gamescope-session.service -p NRestarts --value").strip() == "0"
finally:
    if client is not None:
        client.disconnect()
    api.shutdown()
    print(machine.succeed("journalctl -b --no-pager -u display-manager -t gamescope -t gamescope-session | tail -n 120"))
    print(machine.succeed("journalctl -b --no-pager _UID=1000 | tail -n 180"))
