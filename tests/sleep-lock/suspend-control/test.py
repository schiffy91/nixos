import time
import shlex
import base64
from pathlib import Path

start_all()
machine.wait_for_unit("multi-user.target")
machine.wait_until_succeeds("pgrep -u observer plasmashell")
environment_command = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user show-environment"
machine.wait_until_succeeds(environment_command + " | grep '^WAYLAND_DISPLAY='", timeout=60)
environment = dict(line.split("=", 1) for line in machine.succeed(environment_command).splitlines() if "=" in line)
display = environment["WAYLAND_DISPLAY"]
machine.wait_for_file("/run/user/1000/" + display, timeout=30)
machine.succeed("systemd-run --unit=test-terminal --uid=observer --setenv=XDG_RUNTIME_DIR=/run/user/1000 --setenv=WAYLAND_DISPLAY=" + shlex.quote(display) + " --setenv=DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus konsole --separate")
machine.wait_for_text("Konsole", timeout=30)
machine.send_chars("touch /tmp/control-before-suspend\n")
machine.wait_for_file("/tmp/control-before-suspend")
machine.succeed("test ! -e /run/nixos-sleep-lock/state.json")
print(machine.succeed("cat /proc/driver/rtc; cat /sys/class/rtc/rtc0/device/power/wakeup; cat /sys/power/mem_sleep"))
machine.succeed("echo 1 > /sys/power/pm_debug_messages; echo 1 > /sys/power/pm_print_times; dmesg -n 8; touch /run/real-suspend")
started = time.monotonic()
machine.succeed("systemctl suspend --no-block")
machine.wait_for_console_text(r"PM: suspend entry \(s2idle\)", timeout=30)
machine.wait_for_console_text("PM: suspend-to-idle", timeout=30)
time.sleep(5)
assert "PM: suspend exit" not in machine.get_console_log()
assert machine.qmp_client is not None
machine.qmp_client.send("system_powerdown")
try:
    machine.wait_for_console_text("PM: suspend exit", timeout=30)
except Exception:
    print(repr(machine.send_monitor_command("info status")))
    raise
assert time.monotonic() - started >= 5
machine.wait_until_succeeds("test $(systemctl show -p SubState --value systemd-suspend.service) = dead", timeout=30)
machine.succeed("test $(systemctl show -p Result --value systemd-suspend.service) = success")
machine.wait_until_succeeds("pgrep -u observer plasmashell")
try:
    machine.send_key("shift")
    machine.wait_for_text("Konsole", timeout=30)
    machine.send_chars("touch /tmp/control-after-suspend\n")
    machine.wait_for_file("/tmp/control-after-suspend", timeout=10)
except Exception:
    machine.screenshot("control-resume-failure")
    print("FAILED-CONTROL-PNG:" + base64.b64encode((Path(machine.out_dir) / "control-resume-failure.png").read_bytes()).decode())
    print(machine.succeed("cat /sys/class/tty/tty0/active; cat /sys/class/drm/card*-*/status; runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 WAYLAND_DISPLAY=" + shlex.quote(display) + " kscreen-doctor -o; journalctl -b -k --no-pager | grep -E 'drm|virtio|vga'"))
    raise
