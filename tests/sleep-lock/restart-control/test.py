import json
import shlex
import time

start_all()
control = json.loads(machine.succeed("cat /etc/restart-control-packages.json"))
print("CONTROL PACKAGES", control)
machine.wait_for_unit("multi-user.target")
machine.wait_until_succeeds("pgrep -u observer plasmashell")
user = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus "
env_cmd = user + "systemctl --user show-environment"
machine.wait_until_succeeds(env_cmd + " | grep '^WAYLAND_DISPLAY='", timeout=60)
environment = dict(line.split("=", 1) for line in machine.succeed(env_cmd).splitlines() if "=" in line)
user += "WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "
run_user = user + "systemd-run --user --setenv=WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "
machine.succeed(run_user + "--unit=control-probe sleep-capture-probe /tmp/sleep-capture.ppm")
machine.wait_until_succeeds("test -s /tmp/sleep-capture.ppm", timeout=60)
outputs = json.loads(machine.succeed(user + "kscreen-doctor -j"))["outputs"]
physical = [o["name"] for o in outputs if o["name"] != "Virtual-Sleep capture test"]
assert len(physical) == 1 and len(outputs) == 2, outputs
sessions = json.loads(machine.succeed("loginctl list-sessions --json=short"))
session = next(s["session"] for s in sessions if s["uid"] == 1000 and s["seat"] == "seat0")
kwin_pattern = r"^/nix/store/.*/bin/\.?kwin_wayland(-wrapped)?( |$)"
find_kwin = "pgrep -u observer -f " + shlex.quote(kwin_pattern)
original_vt = machine.succeed("cat /sys/class/tty/tty0/active").strip()
assert original_vt.startswith("tty") and original_vt != "tty63"

for cycle in range(2):
    with subtest(f"inactive compositor restart {cycle + 1} without the root sleep-lock boundary"):
        old_pid = machine.succeed(find_kwin).strip()
        assert old_pid.isdigit(), old_pid
        machine.succeed("test ! -e /run/nixos-sleep-lock/state.json")
        machine.succeed("chvt 63")
        machine.wait_until_succeeds("test $(cat /sys/class/tty/tty0/active) = tty63")
        print("INACTIVE SESSION", machine.succeed("loginctl show-session " + shlex.quote(session)))
        access = "runuser -u observer -- test"
        if control["video"]:
            machine.wait_until_succeeds(access + " -r /dev/dri/card0 && " + access + " -w /dev/dri/card0")
        else:
            machine.wait_until_succeeds("! " + access + " -r /dev/dri/card0 && ! " + access + " -w /dev/dri/card0")
        machine.succeed("kill -KILL " + old_pid)
        machine.wait_until_succeeds("test $(" + find_kwin + ") != " + old_pid, timeout=30)
        time.sleep(3)
        print("INACTIVE OUTPUTS", machine.execute(user + "kscreen-doctor -j"))
        machine.succeed("chvt " + original_vt[3:])
        machine.wait_until_succeeds("test $(cat /sys/class/tty/tty0/active) = " + original_vt)
        for attempt in range(60):
            result = machine.execute(user + "kscreen-doctor -j")
            if result[0] == 0:
                outputs = json.loads(result[1])["outputs"]
                restored = [o for o in outputs if o["name"] in physical]
                if len(restored) == 1 and restored[0]["connected"]:
                    break
            time.sleep(.25)
        else:
            print("RESTORED SESSION", machine.succeed("loginctl show-session " + shlex.quote(session)))
            print("DRM PERMISSIONS", machine.execute("ls -l /dev/dri; getfacl /dev/dri/card0"))
            print("COMPOSITOR LOG", machine.execute("journalctl -b _UID=1000 --no-pager -n 100"))
            raise AssertionError(f"Physical output missing after inactive restart without root locking: {outputs}")
        machine.succeed("test ! -e /run/nixos-sleep-lock/state.json")
        print("RESTORED OUTPUTS", outputs)
