import json
import shlex
import time
from pathlib import Path
from PIL import Image

start_all()
machine.wait_for_unit("multi-user.target")
machine.wait_until_succeeds("test -p /run/test-input")
machine.wait_until_succeeds("pgrep -u observer plasmashell")
environment_command = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user show-environment"
machine.wait_until_succeeds(environment_command + " | grep '^WAYLAND_DISPLAY='", timeout=60)
environment = dict(line.split("=", 1) for line in machine.succeed(environment_command).splitlines() if "=" in line)
display = environment["WAYLAND_DISPLAY"]
machine.wait_for_file("/run/user/1000/" + display, timeout=30)
machine.succeed("systemd-run --unit=test-terminal --uid=observer --setenv=XDG_RUNTIME_DIR=/run/user/1000 --setenv=WAYLAND_DISPLAY=" + shlex.quote(display) + " --setenv=DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus konsole --separate")
machine.wait_for_text("Konsole", timeout=30)
machine.send_chars("touch /tmp/session-before\n")
machine.wait_for_file("/tmp/session-before", timeout=10)
sessions = json.loads(machine.succeed("loginctl list-sessions --json=short"))
session = next(s["session"] for s in sessions if s["uid"] == 1000 and s["seat"] == "seat0")
original_vt = machine.succeed("loginctl show-session " + shlex.quote(session) + " -p VTNr --value").strip()
assert 0 < int(original_vt) < 63

with subtest("compositor survives an ordinary console round trip"):
    machine.succeed("test-vt lock")
    time.sleep(1)
    machine.succeed("test-vt unlock; chvt " + original_vt)
    try:
        machine.wait_for_text("Konsole", timeout=20)
    finally:
        machine.screenshot("console-control")

machine.succeed("test-vt lock")


def locked():
    assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"


def cannot_switch():
    # VT_ACTIVATE can return success without changing consoles while locked.
    machine.execute("timeout 2 chvt " + original_vt)
    machine.succeed("loginctl activate " + shlex.quote(session))
    machine.send_key("ctrl-alt-f" + original_vt)
    time.sleep(.5)
    locked()


def prompt():
    machine.wait_until_fails("systemctl is-active test-password.service")
    machine.succeed("rm -f /run/test-answer")
    machine.wait_until_succeeds("plymouth --ping")
    machine.succeed("plymouth show-splash")
    machine.succeed("systemd-run --unit=test-password --collect --property=StandardOutput=file:/run/test-answer plymouth ask-for-password --prompt='Account password'")
    for _ in range(30):
        machine.screenshot("lock-keyboard")
        img = Image.open(Path(machine.out_dir) / "lock-keyboard.png").convert("RGB")
        counts = img.getcolors(img.width * img.height)
        assert counts is not None
        colors = dict((color, count) for count, color in counts)
        if colors.get((0x23, 0x26, 0x2e), 0) > img.width * 10:
            return img.size
        time.sleep(.5)
    raise AssertionError("Plymouth keyboard did not appear on the secured console")


def touch(x, y):
    machine.succeed("echo " + shlex.quote(f"down {int(x * 32767)} {int(y * 32767)}") + " > /run/test-input")
    time.sleep(.15)
    machine.succeed("echo up > /run/test-input")
    time.sleep(.15)


def key(name, size):
    # Independent XKB physical-key geometry, shared with the boot UI contract.
    positions = {"a": (7, 8, 4), "enter": (42, 16, 18)}
    x, y, width = positions[name]
    screen_width, screen_height = size
    kw, kh = min(int(screen_width * .96), 1280), screen_height * 52 // 100
    touch(((screen_width - kw) // 2 + kw * (x + width / 2) / 60) / screen_width,
          (screen_height - kh + 4 + (kh - 8) * (y + 2) / 20) / screen_height)


with subtest("boundary survives the acquiring process exiting"):
    locked()
    cannot_switch()
    machine.fail("su -s /bin/sh observer -c 'test-vt unlock'")
    # Keep root's access to the device, but drop the specific kernel capability.
    # This proves the ioctl restriction as well as the device's ownership.
    restricted = "setpriv --bounding-set=-sys_tty_config test-vt "
    assert machine.succeed(restricted + "state").strip() == "63"
    machine.fail(restricted + "unlock")
    machine.send_chars("touch /tmp/session-bypass\n")
    machine.succeed("test ! -e /tmp/session-bypass")
    locked()

with subtest("touch UI on a secured VT"):
    machine.succeed("systemctl start test-lock-ui")
    size = prompt()
    machine.fail("timeout 2 su -s /bin/sh observer -c 'plymouth ask-for-password'")
    cannot_switch()
    key("a", size)
    key("enter", size)
    machine.wait_until_succeeds("test -s /run/test-answer")
    assert machine.succeed("cat /run/test-answer").strip() == "a"
    cannot_switch()
    machine.send_key("esc")
    cannot_switch()

with subtest("crash and restart never release the console"):
    machine.succeed("systemctl kill -s SIGKILL test-lock-ui")
    cannot_switch()
    machine.succeed("test-vt relock")
    machine.wait_for_unit("test-lock-ui")
    size = prompt()
    cannot_switch()
    machine.send_chars("b\n")
    machine.wait_until_succeeds("test -s /run/test-answer")
    assert machine.succeed("cat /run/test-answer").strip() == "b"
    cannot_switch()

with subtest("quitting the UI does not release the console"):
    machine.succeed("systemctl stop test-lock-ui")
    cannot_switch()
    machine.succeed("test-vt unlock; chvt " + original_vt)
    assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty" + original_vt
    machine.wait_until_succeeds("test \"$(loginctl show-session " + shlex.quote(session) + " -p Active --value)\" = yes")
    # The session must actually render again after logind resumes its devices.
    try:
        machine.wait_for_text("Konsole", timeout=30)
    finally:
        machine.screenshot("returned-session")
    machine.send_chars("touch /tmp/session-after\n")
    try:
        machine.wait_for_file("/tmp/session-after", timeout=10)
    finally:
        machine.screenshot("returned-session-input")
    machine.succeed("test ! -e /tmp/session-bypass")
