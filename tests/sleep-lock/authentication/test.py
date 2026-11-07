import base64
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
    machine.wait_until_succeeds("pgrep -f '^/nix/store/[^ ]+/bin/sleep-lock-native authenticate observer( retry)?$'")
    for _ in range(40):
        machine.screenshot("authentication-keyboard")
        img = Image.open(Path(machine.out_dir) / "authentication-keyboard.png").convert("RGB")
        counts = img.getcolors(img.width * img.height)
        assert counts is not None
        colors = dict((color, count) for count, color in counts)
        if colors.get((0x23, 0x26, 0x2e), 0) > img.width * 10:
            return img.size
        time.sleep(.25)
    print("FAILED-KEYBOARD-PNG:" + base64.b64encode((Path(machine.out_dir) / "authentication-keyboard.png").read_bytes()).decode())
    print(machine.succeed("cat /sys/class/tty/tty0/active; systemctl status nixos-sleep-lock --no-pager"))
    raise AssertionError("Password keyboard is absent")


def restored():
    machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json")
    assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty" + original_vt
    machine.wait_for_text("Konsole", timeout=30)


def begin(fail=False):
    machine.succeed("rm -f /run/sleep-backend-entered /run/fail-sleep-backend")
    if fail:
        machine.succeed("touch /run/fail-sleep-backend")
    machine.succeed("systemctl reset-failed")
    (machine.fail if fail else machine.succeed)("systemctl start systemd-suspend.service")
    machine.wait_for_file("/run/sleep-backend-entered", timeout=30)
    return prompt()


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


with subtest("sleep backend runs only behind an acknowledged console lock"):
    size = begin()
    cannot_switch()
    machine.fail("su -s /bin/sh observer -c 'cat /run/nixos-sleep-lock/state.json'")
    machine.fail("runuser -u observer -- $(cut -d ' ' -f1 /etc/sleep-lock-controller) resume systemd-suspend.service")
    machine.send_chars("wrong\n")
    time.sleep(3)
    cannot_switch()
    size = prompt()
    key("a", size)
    key("enter", size)
    restored()
    machine.send_chars("touch /tmp/after-first-lock\n")
    machine.wait_for_file("/tmp/after-first-lock")

with subtest("failed sleep still requires the account password"):
    begin(fail=True)
    cannot_switch()
    machine.send_chars("a\n")
    restored()

with subtest("a real logind Plasma X11 session refuses sleep admission"):
    # No X server is needed to test admission: pam_systemd registers the actual
    # session metadata that the production bridge obtains from logind.
    machine.succeed("systemd-run --unit=x11-admission --property=User=observer --property=PAMName=login --setenv=XDG_SESSION_TYPE=x11 --setenv=XDG_SESSION_DESKTOP=KDE --setenv=XDG_SESSION_CLASS=user /run/current-system/sw/bin/sleep infinity")
    deadline = time.monotonic() + 15
    while True:
        candidates = json.loads(machine.succeed("loginctl list-sessions --json=short"))
        x11_sessions = [entry["session"] for entry in candidates
                        if entry["uid"] == 1000 and entry["session"] != session and
                        machine.succeed("loginctl show-session " + shlex.quote(entry["session"]) + " -p Type --value").strip() == "x11"]
        if x11_sessions:
            break
        assert time.monotonic() < deadline, "PAM did not register the X11 fixture session"
        time.sleep(.1)
    machine.succeed("rm -f /run/sleep-backend-entered; systemctl reset-failed")
    machine.fail("systemctl start systemd-suspend.service")
    machine.succeed("test ! -e /run/sleep-backend-entered; journalctl -b -u systemd-suspend.service --no-pager | grep -F 'Sleep locking supports Plasma Wayland only'")
    cannot_switch()
    machine.succeed("systemctl stop x11-admission.service")
    for identifier in x11_sessions:
        machine.wait_until_fails("loginctl show-session " + shlex.quote(identifier))
    prompt()
    machine.send_chars("a\n")
    restored()

with subtest("crashed display and controller retain the boundary and recover"):
    begin()
    machine.succeed("pkill -KILL -x plymouthd")
    cannot_switch()
    prompt()
    machine.succeed("systemctl kill --kill-whom=all -s SIGKILL nixos-sleep-lock")
    cannot_switch()
    prompt()
    machine.send_chars("a\n")
    restored()

with subtest("changing the configured password account cannot release an existing lock"):
    begin()
    state = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
    assert state["uid"] == 1000
    source = machine.succeed("cat $(cut -d ' ' -f1 /etc/sleep-lock-controller)")
    user_line = next(line for line in source.splitlines() if line.startswith("USER = "))
    altered = source.replace(user_line, 'USER = "root"', 1)
    machine.succeed("systemctl stop nixos-sleep-lock; mkdir -p /run/systemd/system/nixos-sleep-lock.service.d")
    machine.succeed("printf %s " + shlex.quote(altered) + " > /run/account-fault-controller; chmod 700 /run/account-fault-controller")
    override = "[Service]\nExecStart=\nExecStart=/run/account-fault-controller run\n"
    machine.succeed("printf %s " + shlex.quote(override) + " > /run/systemd/system/nixos-sleep-lock.service.d/account.conf; systemctl daemon-reload; systemctl start --no-block nixos-sleep-lock")
    machine.wait_until_succeeds("journalctl -b -u nixos-sleep-lock --no-pager | grep -F 'The configured password account changed'")
    machine.fail("pgrep -f '^/nix/store/[^ ]+/bin/sleep-lock-native authenticate '")
    cannot_switch()
    assert json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json")) == state
    machine.succeed("systemctl stop nixos-sleep-lock; rm /run/systemd/system/nixos-sleep-lock.service.d/account.conf; systemctl daemon-reload; systemctl reset-failed; systemctl start nixos-sleep-lock")
    prompt()
    machine.send_chars("a\n")
    restored()
    machine.succeed("rm /run/account-fault-controller")

with subtest("only root can control native sleep-lock authority"):
    begin()
    query = "runuser -u observer -- busctl --address=unix:path=/run/user/1000/bus call org.kde.screensaver /ScreenSaver org.kde.screensaver "
    dummy = "00000000-0000-0000-0000-000000000000"
    machine.fail(query + "LockFromRoot")
    machine.fail(query + "IsRootLockCurrent s " + dummy)
    machine.fail(query + "UnlockFromRoot s " + dummy)
    # logind accepts this from the session UID, but the opt-in native package
    # must not convert its unversioned signal into release authority.
    machine.succeed("runuser -u observer -- loginctl unlock-session " + shlex.quote(session))
    time.sleep(1)
    cannot_switch()
    native_query = "runuser -u observer -- busctl --address=unix:path=/run/user/1000/bus call org.freedesktop.ScreenSaver /ScreenSaver org.freedesktop.ScreenSaver GetActive"
    assert machine.succeed(native_query).strip() == "b true"
    prompt()
    machine.send_chars("a\n")
    restored()

with subtest("a later sleep invalidates a password prompt already in progress"):
    begin()
    # The first prompt is already awaiting input when another sleep begins.
    machine.succeed("touch /run/hold-sleep-backend; rm /run/sleep-backend-entered")
    machine.succeed("systemctl start --no-block systemd-suspend.service")
    machine.wait_for_file("/run/sleep-backend-entered", timeout=30)
    machine.send_chars("a\n")
    time.sleep(1)
    cannot_switch()
    machine.succeed("rm /run/hold-sleep-backend")
    machine.wait_until_fails("systemctl is-active systemd-suspend.service")
    prompt()
    cannot_switch()
    machine.send_chars("a\n")
    restored()

with subtest("cancelled sleep remains locked"):
    machine.succeed("touch /run/hold-sleep-backend; rm /run/sleep-backend-entered")
    machine.succeed("systemctl start --no-block systemd-suspend.service")
    machine.wait_for_file("/run/sleep-backend-entered", timeout=30)
    machine.succeed("systemctl stop systemd-suspend.service; rm /run/hold-sleep-backend")
    prompt()
    cannot_switch()
    machine.send_chars("a\n")
    restored()

with subtest("password material is absent from state and journal"):
    machine.succeed("test ! -e /run/nixos-sleep-lock/state.json")
    # A long distinctive wrong password is searchable without ambiguity.
    begin()
    machine.send_chars("UNIQUE-REJECTED-PASSWORD-64196\n")
    time.sleep(3)
    logs = machine.succeed("journalctl -b --no-pager")
    assert "UNIQUE-REJECTED-PASSWORD-64196" not in logs
    machine.succeed("test $(stat -c %a /run/nixos-sleep-lock) = 700")
    machine.succeed("test $(stat -c %a /run/nixos-sleep-lock/state.json) = 600")
    prompt()
    machine.send_chars("a\n")
    restored()
    machine.screenshot("authenticated-session")

with subtest("an unavailable lock service prevents the sleep backend"):
    machine.succeed("systemctl stop nixos-sleep-lock; mkdir -p /run/systemd/system/nixos-sleep-lock.service.d")
    override = "[Service]\nExecStart=\nExecStart=/run/current-system/sw/bin/false\nRestart=no\n"
    machine.succeed("printf %s " + shlex.quote(override) + " > /run/systemd/system/nixos-sleep-lock.service.d/failure.conf")
    machine.succeed("systemctl daemon-reload")
    machine.succeed("rm /run/sleep-backend-entered")
    machine.fail("systemctl start systemd-suspend.service")
    machine.succeed("test ! -e /run/sleep-backend-entered")
    cannot_switch()
    machine.succeed("rm /run/systemd/system/nixos-sleep-lock.service.d/failure.conf; systemctl daemon-reload; systemctl reset-failed; systemctl start nixos-sleep-lock")
    prompt()
    machine.send_chars("a\n")
    restored()

with subtest("killing the authenticator cannot authorize unlock"):
    begin()
    machine.succeed("pkill -KILL -f '^/nix/store/[^ ]+/bin/sleep-lock-native authenticate observer( retry)?$'")
    cannot_switch()
    prompt()
    machine.send_chars("a\n")
    restored()

with subtest("unprivileged Plymouth clients cannot read passwords or dismiss the display"):
    begin()
    machine.send_chars("IPC-REJECTED-PASSWORD-57120\n")
    time.sleep(3)
    prompt()
    daemon = machine.succeed("pgrep -x plymouthd").strip()
    # The CLI's --command fallback can succeed even after a denied request.
    # Verify the actual daemon protocol response, without running that fallback.
    interpreter = machine.succeed("head -1 $(cut -d ' ' -f1 /etc/sleep-lock-controller)").strip()[2:]
    client = "\n".join([
        "import socket",
        "for request in (b'c\\0', b'Q\\0'):",
        "    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:",
        "        connection.settimeout(2)",
        "        connection.connect('\\0/org/freedesktop/plymouthd')",
        "        connection.sendall(request)",
        "        assert connection.recv(1) == b'\\x15', 'Expected permission denial'",
    ])
    machine.succeed("runuser -u observer -- " + shlex.quote(interpreter) + " -c " + shlex.quote(client))
    assert machine.succeed("pgrep -x plymouthd").strip() == daemon
    cannot_switch()
    machine.send_chars("a\n")
    restored()

with subtest("interrupted lock acquisition remains recoverable and cannot enter sleep"):
    # Inject interruption around the real native operation in a VM-only copy.
    # The service's original ExecStopPost and controller stay unmodified.
    source = machine.succeed("cat $(cut -d ' ' -f1 /etc/sleep-lock-controller)")
    native_line = next(line for line in source.splitlines() if line.startswith("NATIVE = "))
    native = json.loads(native_line.split(" = ", 1)[1])
    interpreter = source.splitlines()[0]
    wrapper = interpreter + "\n" + "\n".join([
        "import os, subprocess, sys, time",
        "from pathlib import Path",
        "native = " + repr(native),
        "if sys.argv[1] == 'secure':",
        "    if Path('/run/sleep-lock-fault-stage').read_text() == 'after':",
        "        subprocess.run([native, *sys.argv[1:]], check=True)",
        "    Path('/run/sleep-lock-fault-ready').touch()",
        "    while True:",
        "        time.sleep(1)",
        "os.execv(native, [native, *sys.argv[1:]])",
        "",
    ])
    altered = source.replace(native_line, 'NATIVE = "/run/sleep-lock-fault-native"', 1)
    for path, content in [("/run/sleep-lock-fault-native", wrapper), ("/run/sleep-lock-fault-controller", altered)]:
        machine.succeed("printf %s " + shlex.quote(content) + " > " + path + "; chmod 700 " + path)
    machine.succeed("mkdir -p /run/systemd/system/systemd-suspend.service.d")
    override = "[Service]\nExecStartPre=\nExecStartPre=/run/sleep-lock-fault-controller prepare %n\n"
    for stage in ["before", "after"]:
        machine.succeed("systemctl stop nixos-sleep-lock; systemctl reset-failed")
        machine.succeed("rm -f /run/sleep-lock-fault-ready /run/sleep-backend-entered /run/fail-sleep-backend")
        machine.succeed("printf %s " + shlex.quote(stage) + " > /run/sleep-lock-fault-stage")
        machine.succeed("printf %s " + shlex.quote(override) + " > /run/systemd/system/systemd-suspend.service.d/interruption.conf")
        machine.succeed("systemctl daemon-reload; systemctl start --no-block systemd-suspend.service")
        machine.wait_for_file("/run/sleep-lock-fault-ready", timeout=10)
        state = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
        assert state["phase"] == "acquiring"
        active = machine.succeed("cat /sys/class/tty/tty0/active").strip()
        assert active == ("tty63" if stage == "after" else "tty" + original_vt)
        machine.succeed("systemctl kill --kill-whom=all -s SIGKILL systemd-suspend.service")
        machine.wait_until_succeeds("test $(systemctl show -p SubState --value systemd-suspend.service) = failed", timeout=20)
        machine.succeed("test ! -e /run/sleep-backend-entered")
        machine.succeed("rm /run/systemd/system/systemd-suspend.service.d/interruption.conf; systemctl daemon-reload")
        prompt()
        cannot_switch()
        state = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
        assert state["phase"] == "locked" and state["holds"] == []
        machine.send_chars("wrong\n")
        time.sleep(3)
        cannot_switch()
        prompt()
        machine.send_chars("a\n")
        restored()
    machine.succeed("rm /run/sleep-lock-fault-{native,controller,stage,ready}")

with subtest("interrupted release requires one fresh password for the later native lock"):
    source = machine.succeed("cat $(cut -d ' ' -f1 /etc/sleep-lock-controller)")
    native_line = next(line for line in source.splitlines() if line.startswith("NATIVE = "))
    native = json.loads(native_line.split(" = ", 1)[1])
    wrapper = source.splitlines()[0] + "\n" + "\n".join([
        "import os, sys, time",
        "from pathlib import Path",
        "native = " + repr(native),
        "if sys.argv[1] == 'release':",
        "    Path('/run/release-interrupted').touch()",
        "    while True: time.sleep(1)",
        "os.execv(native, [native, *sys.argv[1:]])",
        "",
    ])
    altered = source.replace(native_line, 'NATIVE = "/run/release-fault-native"', 1)
    for path, content in [("/run/release-fault-native", wrapper), ("/run/release-fault-controller", altered)]:
        machine.succeed("printf %s " + shlex.quote(content) + " > " + path + "; chmod 700 " + path)
    machine.succeed("systemctl stop nixos-sleep-lock; mkdir -p /run/systemd/system/nixos-sleep-lock.service.d")
    override = "[Service]\nExecStart=\nExecStart=/run/release-fault-controller run\n"
    machine.succeed("printf %s " + shlex.quote(override) + " > /run/systemd/system/nixos-sleep-lock.service.d/release.conf; systemctl daemon-reload")
    begin()
    machine.send_chars("a\n")
    machine.wait_for_file("/run/release-interrupted")
    state = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
    assert state["phase"] == "authenticated"
    old_epoch = state["epoch"]
    native_query = "runuser -u observer -- busctl --address=unix:path=/run/user/1000/bus call org.freedesktop.ScreenSaver /ScreenSaver org.freedesktop.ScreenSaver GetActive"
    assert machine.succeed(native_query).strip() == "b false"
    machine.succeed("loginctl lock-session " + shlex.quote(session))
    machine.wait_until_succeeds(native_query + " | grep -Fx 'b true'")
    machine.succeed("rm /run/systemd/system/nixos-sleep-lock.service.d/release.conf; systemctl daemon-reload; systemctl kill --kill-whom=all -s SIGKILL nixos-sleep-lock")
    prompt()
    cannot_switch()
    state = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
    assert state["epoch"] != old_epoch and state["phase"] == "locked"
    assert state["plasma"][0]["id"] == session
    assert machine.succeed(native_query).strip() == "b true"
    machine.send_chars("wrong\n")
    time.sleep(3)
    cannot_switch()
    assert machine.succeed(native_query).strip() == "b true"
    prompt()
    machine.send_chars("a\n")
    machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json")
    # This fresh account-password success releases the exact current session;
    # there is no second password prompt from the native Plasma lock.
    machine.wait_until_succeeds(native_query + " | grep -Fx 'b false'")
    restored()
    machine.send_chars("touch /tmp/after-interrupted-release\n")
    machine.wait_for_file("/tmp/after-interrupted-release")
    machine.succeed("rm /run/release-fault-native /run/release-fault-controller /run/release-interrupted")

with subtest("actual hibernation helper failure leaves the session locked"):
    machine.succeed("test $(wc -l < /proc/swaps) = 1")
    machine.fail("systemctl start systemd-hibernate.service")
    prompt()
    cannot_switch()
    machine.send_chars("a\n")
    restored()

with subtest("power-button-woken s2idle preserves the password boundary"):
    machine.succeed("echo 1 > /sys/power/pm_debug_messages; dmesg -n 8; touch /run/real-suspend; systemctl reset-failed")
    previous_epoch = None
    for cycle in range(2):
        before_idle = machine.get_console_log().count("PM: suspend-to-idle")
        before_exit = machine.get_console_log().count("PM: suspend exit")
        machine.succeed("systemctl suspend --no-block")
        deadline = time.monotonic() + 30
        while machine.get_console_log().count("PM: suspend-to-idle") == before_idle:
            assert time.monotonic() < deadline, "Kernel did not enter s2idle"
            time.sleep(.1)
        time.sleep(5)
        assert machine.get_console_log().count("PM: suspend exit") == before_exit
        assert machine.qmp_client is not None
        machine.qmp_client.send("system_powerdown")
        deadline = time.monotonic() + 30
        while machine.get_console_log().count("PM: suspend exit") == before_exit:
            assert time.monotonic() < deadline, "Kernel did not resume"
            time.sleep(.1)
        machine.wait_until_succeeds("test $(systemctl show -p SubState --value systemd-suspend.service) = dead", timeout=30)
        machine.succeed("test $(systemctl show -p Result --value systemd-suspend.service) = success")
        prompt()
        cannot_switch()
        state = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
        assert state["epoch"] != previous_epoch
        previous_epoch = state["epoch"]
        if cycle == 0:
            # Leave the password prompt open with pre-sleep input. A second
            # actual sleep must cancel that attempt and discard its input.
            machine.send_chars("a")
        else:
            machine.send_chars("\n")
            time.sleep(3)
            cannot_switch()
            prompt()
    machine.send_chars("a\n")
    restored()
    machine.send_chars("touch /tmp/after-real-suspend\n")
    machine.wait_for_file("/tmp/after-real-suspend")
    machine.screenshot("resumed-session")
    machine.succeed("rm /run/real-suspend")

with subtest("a correct password cannot unlock an expired account"):
    begin()
    machine.succeed("chage -E 0 observer")
    machine.send_chars("a\n")
    time.sleep(3)
    cannot_switch()
    machine.succeed("chage -E -1 observer")
    prompt()
    machine.send_chars("a\n")
    restored()
