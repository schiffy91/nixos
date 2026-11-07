import json
import os
import re
import shlex
import time
from PIL import Image

start_all()
machine.wait_until_succeeds("pgrep -u observer plasmashell")
client.wait_for_x()
user = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus "
client_user = "runuser -u viewer -- env DISPLAY=:0 XAUTHORITY=/home/viewer/.Xauthority XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus "
machine.wait_until_succeeds(user + "systemctl --user show-environment | grep '^WAYLAND_DISPLAY='")
environment = dict(line.split("=", 1) for line in machine.succeed(user + "systemctl --user show-environment").splitlines() if "=" in line)
user += "WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "
run_user = user + "systemd-run --user --setenv=WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "


def api(endpoint, payload=None):
    command = "python /etc/sunshine-api.py " + shlex.quote(endpoint)
    if payload is not None:
        command += " " + shlex.quote(json.dumps(payload))
    return json.loads(machine.succeed(command))


frame_number = 0


def decoded_frame():
    global frame_number
    frame_number += 1
    name = f"decoded-{frame_number:03}"
    client.screenshot(name)
    image = Image.open(os.path.join(client.out_dir, name + ".png")).convert("RGB")
    colors = image.getcolors(image.width * image.height)
    assert colors is not None
    private = 0
    for count, color in colors:
        assert isinstance(color, tuple) and len(color) == 3
        r, g, b = color
        if r > 100 and g < 65 and b < 65:
            private += count
    return private / (image.width * image.height)


def wait_decoded(private):
    for _ in range(40):
        fraction = decoded_frame()
        matches = fraction > .4 if private else fraction < .01
        if matches:
            return
        time.sleep(.5)
    raise AssertionError(f"Decoded private={private}, last private pixel fraction={fraction}")


def capture_serials():
    journal = machine.succeed(user + "journalctl --user -b -u sunshine --no-pager")
    return set(re.findall(r"\[kwingrab\] Pipewire stream created: objectserial=(\d+)", journal))


try:
    machine.succeed(run_user + "--unit=capture-probe sleep-capture-probe /tmp/sleep-capture.ppm")
    machine.wait_until_succeeds("test -s /tmp/sleep-capture.ppm")
    outputs = json.loads(machine.succeed(user + "kscreen-doctor -j"))["outputs"]
    target = next(output for output in outputs if output["name"] == "Virtual-Sleep capture test")
    machine.succeed(user + f"kscreen-doctor output.{target['id']}.position.1280,0")
    machine.succeed(run_user + "--unit=sunshine-private sunshine-private-client")
    machine.wait_until_succeeds("test -s /tmp/sunshine-private-tick")
    machine.succeed(user + "pactl load-module module-null-sink sink_name=fixture")
    # Public disposable fixture credentials; no production vault or password.
    provision = """
import hashlib, json, os
from pathlib import Path
d = Path('/home/observer/.config/sunshine')
d.mkdir(parents=True, exist_ok=True)
os.chown(d, 1000, 100); d.chmod(0o700)
p = d / 'credentials.json'
salt = '0123456789abcdef'
p.write_text(json.dumps({'username':'admin','salt':salt,'password':hashlib.sha256(('fixture-password'+salt).encode()).digest()[::-1].hex().upper()}))
os.chown(p, 1000, 100); p.chmod(0o600)
"""
    machine.succeed("python -c " + shlex.quote(provision))
    machine.succeed(user + "systemctl --user start sunshine")
    machine.wait_until_succeeds("python /etc/sunshine-api.py /api/config", timeout=60)
    client.succeed(client_user + "sh -c 'moonlight pair machine --pin 1234 > /tmp/moonlight-pair.log 2>&1 & echo $! > /tmp/moonlight-pair.pid'")
    pending = []
    for _ in range(40):
        pending = api("/api/pin").get("pairings", [])
        if pending:
            break
        time.sleep(.5)
    assert pending, client.succeed("cat /tmp/moonlight-pair.log")
    assert api("/api/pin", {"pairing_id": pending[0]["id"], "pin": "1234", "name": "sleep-test"})["status"]
    paired = "import json; from pathlib import Path; assert json.loads(Path('/home/observer/.config/sunshine/sunshine_state.json').read_text())['root']['named_devices']"
    machine.wait_until_succeeds("python -c " + shlex.quote(paired), timeout=30)
    client.execute('kill "$(cat /tmp/moonlight-pair.pid)"')
    command = "moonlight stream machine Desktop --resolution 1024x768 --fps 30 --bitrate 5000 --video-codec H.264 --video-decoder software --display-mode fullscreen --no-vsync --no-frame-pacing --no-hdr --absolute-mouse"
    client.succeed(client_user + "sh -c " + shlex.quote(command + " > /tmp/moonlight-stream.log 2>&1 & echo $! > /tmp/moonlight-stream.pid"))
    client.wait_for_text("PRIVATE FRAME", timeout=60)
    wait_decoded(True)
    client.send_chars("BEFORE\n")
    machine.wait_until_succeeds("grep -qx BEFORE /tmp/sunshine-private-input", timeout=30)

    with subtest("existing Sunshine stream conceals a live application during root sleep lock"):
        machine.succeed("touch /run/fail-sleep-backend")
        machine.fail("systemctl start systemd-suspend.service")
        machine.wait_for_text("Unlock session", timeout=30)
        machine.wait_until_succeeds("test $(cat /sys/class/tty/tty0/active) = tty63")
        wait_decoded(False)
        before_tick = int(machine.succeed("cat /tmp/sunshine-private-tick"))
        client.send_chars("BLOCKED\n")
        machine.wait_for_text("Authentication failed", timeout=30)
        for _ in range(20):
            assert decoded_frame() < .01
            time.sleep(.25)
        assert int(machine.succeed("cat /tmp/sunshine-private-tick")) > before_tick
        machine.fail("grep -q BLOCKED /tmp/sunshine-private-input")
        machine.succeed("test -e /run/nixos-sleep-lock/state.json")

    with subtest("fresh Linux password through Moonlight restores stream and private input"):
        client.send_chars("a\n")
        machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json", timeout=30)
        wait_decoded(True)
        client.wait_for_text("PRIVATE FRAME", timeout=30)
        client.send_chars("AFTER\n")
        machine.wait_until_succeeds("grep -qx AFTER /tmp/sunshine-private-input", timeout=30)
        machine.fail("grep -q BLOCKED /tmp/sunshine-private-input")
        print("Actual Sunshine KWin capture, Moonlight decode and libvirtualhid keyboard boundary passed")

    with subtest("a new stream from the paired client remains concealed and input-isolated while root locked"):
        machine.fail("systemctl start systemd-suspend.service")
        machine.wait_for_text("Unlock session", timeout=30)
        machine.wait_until_succeeds("test $(cat /sys/class/tty/tty0/active) = tty63")
        wait_decoded(False)
        locked = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
        assert locked["phase"] == "locked" and locked["ready"]
        old_serials = capture_serials()
        assert old_serials
        stream_pid = client.succeed("cat /tmp/moonlight-stream.pid").strip()
        assert stream_pid.isdigit()
        client.succeed("kill " + stream_pid)
        client.wait_until_fails("kill -0 " + stream_pid, timeout=30)
        machine.wait_until_succeeds(user + "journalctl --user -b -u sunshine --no-pager | grep -q 'CLIENT DISCONNECTED'", timeout=30)
        assert json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json")) == locked
        machine.succeed("test $(cat /sys/class/tty/tty0/active) = tty63")

        # Keep the paired identity and virtual-output creator, but require
        # Sunshine to create a fresh KWin/PipeWire capture while root is locked.
        client.succeed(client_user + "sh -c " + shlex.quote(command + " > /tmp/moonlight-reconnect.log 2>&1 & echo $! > /tmp/moonlight-stream.pid"))
        assert client.succeed("cat /tmp/moonlight-stream.pid").strip() != stream_pid
        for _ in range(80):
            if capture_serials() - old_serials:
                break
            time.sleep(.25)
        else:
            raise AssertionError("Reconnected Sunshine did not create a new KWin capture")
        # The native greeter's full date proves decoded server pixels arrived;
        # an empty client desktop or loading window cannot satisfy concealment.
        client.wait_for_text(machine.succeed("LC_ALL=C date '+%A, %B %-d, %Y'").strip(), timeout=60)
        wait_decoded(False)
        client.screenshot("reconnected-locked")
        assert json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json")) == locked
        before_tick = int(machine.succeed("cat /tmp/sunshine-private-tick"))
        client.send_chars("RECONNECTBLOCKED\n")
        machine.wait_for_text("Authentication failed", timeout=30)
        for _ in range(20):
            assert decoded_frame() < .01
            time.sleep(.25)
        assert int(machine.succeed("cat /tmp/sunshine-private-tick")) > before_tick
        machine.fail("grep -q BLOCKED /tmp/sunshine-private-input")
        assert json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json")) == locked
        machine.succeed("test $(cat /sys/class/tty/tty0/active) = tty63")

        client.send_chars("a\n")
        machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json", timeout=30)
        wait_decoded(True)
        client.wait_for_text("PRIVATE FRAME", timeout=30)
        client.send_chars("RECONNECTED\n")
        machine.wait_until_succeeds("grep -qx RECONNECTED /tmp/sunshine-private-input", timeout=30)
        machine.fail("grep -q BLOCKED /tmp/sunshine-private-input")
        client.screenshot("reconnected-unlocked")
        print("Fresh Sunshine capture while root locked conceals pixels and denies private input until Linux password authentication")
finally:
    print("SUNSHINE LOG", machine.execute(user + "journalctl --user -b -u sunshine --no-pager -n 100"))
    print("MOONLIGHT LOG", client.execute("tail -n 100 /tmp/moonlight-stream.log /tmp/moonlight-pair.log /tmp/moonlight-reconnect.log"))
    client.execute('kill "$(cat /tmp/moonlight-stream.pid)" "$(cat /tmp/moonlight-pair.pid)"')
    machine.execute(user + "systemctl --user stop sunshine")
