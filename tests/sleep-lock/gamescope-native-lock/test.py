import json
import shlex
import subprocess
import time
from pathlib import Path
from vncdotool import api
from PIL import Image


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


def locked():
    assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"


def cannot_switch():
    machine.execute("timeout 2 chvt " + original_vt)
    machine.succeed("loginctl activate " + shlex.quote(session))
    machine.send_key("ctrl-alt-f" + original_vt)
    time.sleep(.5)
    locked()


def prompt(label):
    assert client is not None
    machine.wait_until_succeeds("pgrep -f '^/nix/store/[^ ]+/bin/sleep-lock-native authenticate observer( retry)?$'", timeout=30)
    screenshot = Path(machine.out_dir) / f"gamescope-lock-{label}.png"
    for _ in range(40):
        client.captureScreen(str(screenshot), incremental=False)
        with Image.open(screenshot) as source:
            frame = source.convert("RGB")
        counts = frame.getcolors(frame.width * frame.height)
        assert counts is not None
        colors = {color: count for count, color in counts}
        if colors.get((0x23, 0x26, 0x2e), 0) > frame.width * 10:
            print(f"Actual password keyboard frame: {screenshot}", flush=True)
            return frame.size
        time.sleep(.25)
    raise AssertionError(f"Password keyboard absent in {screenshot}")


def begin(label, fail=False):
    machine.succeed("rm -f /run/sleep-backend-entered /run/fail-sleep-backend")
    if fail:
        machine.succeed("touch /run/fail-sleep-backend")
    machine.succeed("systemctl reset-failed")
    (machine.fail if fail else machine.succeed)("systemctl start systemd-suspend.service")
    machine.wait_for_file("/run/sleep-backend-entered", timeout=30)
    return prompt(label)


def touch(x, y):
    machine.succeed("echo " + shlex.quote(f"down {int(x * 32767)} {int(y * 32767)}") + " > /run/test-input")
    time.sleep(.15)
    machine.succeed("echo up > /run/test-input")
    time.sleep(.15)


def key(name, size):
    # Independent physical-key geometry used by the existing Plymouth tests.
    positions = {"a": (7, 8, 4), "enter": (42, 16, 18)}
    x, y, width = positions[name]
    screen_width, screen_height = size
    kw, kh = min(int(screen_width * .96), 1280), screen_height * 52 // 100
    touch(((screen_width - kw) // 2 + kw * (x + width / 2) / 60) / screen_width,
          (screen_height - kh + 4 + (kh - 8) * (y + 2) / 20) / screen_height)


def restored(marker):
    machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json", timeout=30)
    assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty" + original_vt
    assert machine.succeed(f"{user_systemctl} show gamescope-session.service -p MainPID --value").strip() == original_pid
    assert machine.succeed(f"{user_systemctl} show gamescope-session.service -p NRestarts --value").strip() == "0"
    assert client is not None
    client.captureScreen(str(Path(machine.out_dir) / f"gamescope-before-input-{marker}.png"), incremental=False)
    print("Post-unlock DRM state:", machine.execute("cat /sys/kernel/debug/dri/0/state"), flush=True)
    print("Post-unlock focus state:", machine.execute("timeout 3 " + focus_query), flush=True)
    machine.send_chars(f"touch /tmp/{marker}; echo {marker}\n")
    machine.wait_for_file(f"/tmp/{marker}", timeout=10)
    wait_for_frame_text(client, marker, f"gamescope-restored-{marker}")
    print("Restored DRM state:", machine.execute("cat /sys/kernel/debug/dri/0/state"), flush=True)


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
    machine.wait_until_succeeds("test -p /run/test-input", timeout=10)
    session = next(s["session"] for s in sessions if s["uid"] == 1000 and s["seat"] == "seat0")
    original_vt = machine.succeed("loginctl show-session " + shlex.quote(session) + " -p VTNr --value").strip()
    assert 0 < int(original_vt) < 63
    original_pid = machine.succeed(f"{user_systemctl} show gamescope-session.service -p MainPID --value").strip()

    with subtest("Gaming Mode rejects wrong password and accepts touch password"):
        begin("wrong-password")
        cannot_switch()
        # This would create a file if physical input leaked to the game shell.
        machine.send_chars("touch /tmp/INPUTLEAK\n")
        time.sleep(3)
        cannot_switch()
        machine.succeed("test ! -e /tmp/INPUTLEAK")
        size = prompt("touch-password")
        key("a", size)
        key("enter", size)
        restored("AFTERTOUCH")
        machine.succeed("test ! -e /tmp/INPUTLEAK")

    with subtest("failed suspend stays locked and accepts physical keyboard password"):
        begin("failed-backend", fail=True)
        cannot_switch()
        machine.send_chars("a\n")
        restored("AFTERFAILED")
        machine.succeed("rm /run/fail-sleep-backend")

    if real_suspend:
        with subtest("real s2idle returns to password prompt then original Gaming Mode"):
            machine.succeed("echo 1 > /sys/power/pm_debug_messages; dmesg -n 8; touch /run/real-suspend; systemctl reset-failed")
            before_idle = machine.get_console_log().count("PM: suspend-to-idle")
            before_exit = machine.get_console_log().count("PM: suspend exit")
            machine.succeed("systemctl suspend --no-block")
            deadline = time.monotonic() + 30
            while machine.get_console_log().count("PM: suspend-to-idle") == before_idle:
                assert time.monotonic() < deadline, "Kernel did not enter s2idle"
                time.sleep(.1)
            time.sleep(3)
            assert machine.get_console_log().count("PM: suspend exit") == before_exit
            assert machine.qmp_client is not None
            machine.qmp_client.send("system_powerdown")
            deadline = time.monotonic() + 30
            while machine.get_console_log().count("PM: suspend exit") == before_exit:
                assert time.monotonic() < deadline, "Kernel did not resume"
                time.sleep(.1)
            machine.wait_until_succeeds("test $(systemctl show -p SubState --value systemd-suspend.service) = dead", timeout=30)
            machine.succeed("test $(systemctl show -p Result --value systemd-suspend.service) = success")
            prompt("real-suspend")
            cannot_switch()
            machine.send_chars("wrong\n")
            time.sleep(3)
            cannot_switch()
            prompt("after-resume-rejection")
            machine.send_chars("a\n")
            restored("AFTERRESUME")
            machine.succeed("rm /run/real-suspend")

finally:
    if client is not None:
        client.disconnect()
    api.shutdown()
    print(machine.succeed("journalctl -b --no-pager -u nixos-sleep-lock -u systemd-suspend | tail -n 160"))
    print(machine.succeed("journalctl -b --no-pager -u display-manager -t gamescope -t gamescope-session | tail -n 120"))
    print(machine.succeed("journalctl -b --no-pager _UID=1000 | tail -n 180"))
