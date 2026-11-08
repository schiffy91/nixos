import base64
import json
from pathlib import Path
from datetime import timedelta
import shlex
import time

# HMP sendkey holds keys for 100 ms; allow release before the next chord.
key_delay = timedelta(milliseconds=150)

start_all()
machine.wait_for_unit("multi-user.target")
machine.wait_until_succeeds("pgrep -u observer plasmashell", timeout=60)
user_bus = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus busctl --user"
active = user_bus + " call org.freedesktop.ScreenSaver /ScreenSaver org.freedesktop.ScreenSaver GetActive"
machine.wait_until_succeeds(active + " | grep -Fx 'b false'")
sessions = json.loads(machine.succeed("loginctl list-sessions --json=short"))
session = next(s["session"] for s in sessions if s["uid"] == 1000 and s["seat"] == "seat0")
vt = machine.succeed("loginctl show-session " + shlex.quote(session) + " -p VTNr --value").strip()

try:
    with subtest("Gaming-only policy leaves ordinary Plasma password locking intact"):
        machine.succeed("systemctl suspend --no-block")
        machine.wait_for_file("/run/sleep-backend-entered", timeout=30)
        machine.wait_until_succeeds(active + " | grep -Fx 'b true'", timeout=30)
        machine.succeed("test ! -e /run/nixos-sleep-lock/state.json; test ! -e /run/nixos-gaming-locked")
        assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty" + vt
        machine.wait_until_succeeds("pgrep -f kscreenlocker_greet", timeout=15)
        machine.succeed("printf '%s\\n' 'aB3! z' | pamtester kde observer authenticate")
        time.sleep(3)
        machine.send_key("a", delay=key_delay)
        time.sleep(1)
        machine.send_key("ctrl-a", delay=key_delay)
        machine.send_key("backspace", delay=key_delay)
        machine.screenshot("native-plasma-password-prompt")
        machine.send_chars("wrong\n", delay=key_delay)
        machine.wait_until_succeeds("journalctl -b --no-pager | grep -F 'pam_unix(kde:auth): authentication failure'", timeout=30)
        time.sleep(4)
        machine.succeed(active + " | grep -Fx 'b true'")
        machine.send_key("ctrl-a", delay=key_delay)
        machine.send_key("backspace", delay=key_delay)
        machine.send_chars("aB3! z\n", delay=key_delay)
        machine.wait_until_succeeds(active + " | grep -Fx 'b false'", timeout=30)
        machine.succeed("test ! -e /run/nixos-sleep-lock/state.json; test ! -e /run/nixos-gaming-locked")
except Exception:
    machine.screenshot("native-plasma-failure")
    print("PLASMA-SCREENSHOT:" + base64.b64encode((Path(machine.out_dir) / "native-plasma-failure.png").read_bytes()).decode())
    raise
