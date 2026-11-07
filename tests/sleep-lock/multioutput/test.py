import base64
import json
import shlex
import time
from pathlib import Path
from PIL import Image
from test_driver.machine.ocr import perform_ocr_on_screenshot

start_all()
machine.wait_for_unit("multi-user.target")
machine.wait_until_succeeds("pgrep -u observer plasmashell")
user = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus "
environment_command = user + "systemctl --user show-environment"
machine.wait_until_succeeds(environment_command + " | grep '^WAYLAND_DISPLAY='", timeout=60)
environment = dict(line.split("=", 1) for line in machine.succeed(environment_command).splitlines() if "=" in line)
user += "WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "
outputs = json.loads(machine.succeed(user + "kscreen-doctor -j"))["outputs"]
assert len(outputs) == 2 and all(output["connected"] for output in outputs), outputs
position = 0
for output in outputs:
    machine.succeed(user + f"kscreen-doctor output.{output['id']}.enable output.{output['id']}.position.{position},0")
    position += output["size"]["width"]
machine.succeed(user + "systemd-run --user --unit=multioutput-client --setenv=WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " sleep-multioutput-client")


def screens(label):
    assert machine.qmp_client is not None
    result = []
    for index in range(2):
        path = Path(machine.out_dir) / f"{label}-{index}.png"
        raw = path.with_suffix(".ppm")
        machine.qmp_client.send("screendump", {"filename": str(raw), "device": f"graphics{index}"})
        Image.open(raw).save(path)
        raw.unlink()
        result.append(path)
    return result


def desktop(label):
    for attempt in range(30):
        paths = screens(label)
        texts = [perform_ocr_on_screenshot(path) for path in paths]
        backgrounds = set()
        for path in paths:
            image = Image.open(path).convert("RGB")
            counts = image.getcolors(image.width * image.height)
            assert counts is not None
            for count, color in counts:
                if color in [(0x90, 0x20, 0x20), (0x20, 0x60, 0x90)] and count > image.width * image.height // 2:
                    backgrounds.add(color)
        if all("PRIVATE DISPLAY" in text for text in texts) and len(backgrounds) == 2:
            return
        time.sleep(.5)
    raise AssertionError(f"Both actual outputs must render private client content: {texts}")


def locked(label):
    assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"
    for attempt in range(80):
        paths = screens(label)
        covered = []
        diagnostics = []
        for path in paths:
            image = Image.open(path).convert("RGB")
            counts = image.getcolors(image.width * image.height)
            assert counts is not None
            colors = dict((color, count) for count, color in counts)
            private = colors.get((0x90, 0x20, 0x20), 0) + colors.get((0x20, 0x60, 0x90), 0)
            keyboard = colors.get((0x23, 0x26, 0x2e), 0)
            text = perform_ocr_on_screenshot(path)
            covered.append(private == 0 and any(prompt in text for prompt in ["Unlock session", "Authentication failed"]))
            diagnostics.append((image.size, private, keyboard))
        if all(covered) and any(keyboard > size[0] * 10 for size, _, keyboard in diagnostics):
            return next(size for size, _, keyboard in diagnostics if keyboard > size[0] * 10)
        time.sleep(.25)
    print("Lock pixel diagnostics:", diagnostics)
    print(machine.succeed("journalctl -b -u nixos-sleep-lock --no-pager | tail -80"))
    for index, path in enumerate(paths):
        print(f"FAILED-MULTIOUTPUT-{index}:" + base64.b64encode(path.read_bytes()).decode())
    raise AssertionError(f"All DRM outputs must be covered by password UI: {covered}")


def touch_key(name, size):
    positions = {"a": (7, 8, 4), "enter": (42, 16, 18)}
    x, y, width = positions[name]
    screen_width, screen_height = size
    kw, kh = min(int(screen_width * .96), 1280), screen_height * 52 // 100
    tx = ((screen_width - kw) // 2 + kw * (x + width / 2) / 60) / screen_width
    ty = (screen_height - kh + 4 + (kh - 8) * (y + 2) / 20) / screen_height
    machine.succeed("echo " + shlex.quote(f"down {int(tx * 32767)} {int(ty * 32767)}") + " > /run/test-input")
    time.sleep(.15)
    machine.succeed("echo up > /run/test-input")
    time.sleep(.15)


desktop("before")
machine.send_chars("BEFORE\n")
machine.wait_until_succeeds("grep -qx BEFORE /tmp/multioutput-input")
sessions = json.loads(machine.succeed("loginctl list-sessions --json=short"))
session = next(s["session"] for s in sessions if s["uid"] == 1000 and s["seat"] == "seat0")
machine.succeed("systemctl start systemd-suspend.service")
locked("locked")
machine.send_chars("DENIED\n")
time.sleep(3)
machine.succeed("loginctl activate " + shlex.quote(session))
locked("denied")
machine.succeed("test $(wc -l < /tmp/multioutput-input) = 1")
machine.succeed("pkill -KILL -x plymouthd")
size = locked("after-display-crash")
machine.wait_until_succeeds("pgrep -f '^/nix/store/[^ ]+/bin/sleep-lock-native authenticate observer( retry)?$'")
touch_key("a", size)
touch_key("enter", size)
machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json")
desktop("after")
machine.send_chars("AFTER\n")
machine.wait_until_succeeds("grep -qx AFTER /tmp/multioutput-input")
machine.succeed("test $(wc -l < /tmp/multioutput-input) = 2")

with subtest("actual graphical client requests logind suspend with both outputs covered"):
    machine.succeed("echo 1 > /sys/power/pm_debug_messages; echo 1 > /sys/power/pm_print_times; dmesg -n 8; touch /run/real-suspend")
    machine.send_chars("SLEEP\n")
    machine.wait_for_console_text(r"PM: suspend entry \(s2idle\)", timeout=30)
    machine.wait_for_console_text("PM: suspend-to-idle", timeout=30)
    time.sleep(3)
    assert "PM: suspend exit" not in machine.get_console_log()
    assert machine.qmp_client is not None
    machine.qmp_client.send("system_powerdown")
    machine.wait_for_console_text("PM: suspend exit", timeout=30)
    machine.wait_until_succeeds("test $(systemctl show -p SubState --value systemd-suspend.service) = dead", timeout=30)
    machine.send_key("shift")
    locked("after-real-resume")
    machine.send_chars("a\n")
    machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json")
    machine.send_key("shift")
    desktop("after-real-unlock")
    machine.send_chars("RESUMED\n")
    machine.wait_until_succeeds("grep -qx RESUMED /tmp/multioutput-input")
    machine.succeed("test $(wc -l < /tmp/multioutput-input) = 4")


def private_gone(label):
    assert machine.qmp_client is not None
    for index in range(2):
        raw = Path(machine.out_dir) / f"{label}-{index}.ppm"
        try:
            machine.qmp_client.send("screendump", {"filename": str(raw), "device": f"graphics{index}"})
        except Exception as error:
            # Disabled scanout is safe only with independent kernel readback.
            assert "no display device" in str(error).lower(), str(error)
            machine.succeed(f"drm-blank-check /dev/dri/card{index}")
            continue
        image = Image.open(raw).convert("RGB")
        colors = image.getcolors(image.width * image.height)
        assert colors is not None
        assert not any(color in [(0x90, 0x20, 0x20), (0x20, 0x60, 0x90)] for _, color in colors)
        image.save(raw.with_suffix(".png"))
        raw.unlink()


for fault in ["open", "map", "master", "modeset"]:
    with subtest(f"second DRM card {fault} failure denies sleep and recovers while locked"):
        desktop(f"before-{fault}")
        machine.succeed("rm -f /run/real-suspend /run/sleep-backend-entered /run/sleep-renderer-injected")
        machine.succeed("printf %s " + shlex.quote(fault) + " > /run/sleep-renderer-fault")
        machine.succeed("systemctl start --no-block systemd-suspend.service")
        machine.wait_until_succeeds("test -s /run/sleep-renderer-injected", timeout=15)
        # The fault can only fire after the guard has cleared both cards. Watch
        # the whole denial interval, not just a final frame after a UI retry.
        for attempt in range(60):
            private_gone(f"during-{fault}")
            machine.succeed("test ! -e /run/sleep-backend-entered")
            assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"
            if machine.succeed("systemctl show -p Result --value systemd-suspend.service").strip() == "exit-code":
                break
            time.sleep(.5)
        else:
            raise AssertionError("Unsafe display readiness did not reject the sleep request")
        machine.succeed("test -s /run/sleep-renderer-injected; test ! -e /run/sleep-backend-entered")
        assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"
        machine.fail("pgrep -f '^/nix/store/[^ ]+/bin/sleep-lock-native authenticate observer( retry)?$'")
        private_gone(f"failed-{fault}")
        machine.succeed("loginctl activate " + shlex.quote(session))
        private_gone(f"failed-activation-{fault}")
        machine.succeed("test $(wc -l < /tmp/multioutput-input) = 4")
        machine.succeed("rm /run/sleep-renderer-fault")
        locked(f"recovered-{fault}")
        assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"
        machine.succeed("test ! -e /run/sleep-backend-entered")
        machine.send_chars("a\n")
        machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json")
        machine.send_key("shift")
        desktop(f"unlocked-{fault}")
