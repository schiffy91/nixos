import json
import shlex
import time

start_all()
machine.wait_for_unit("multi-user.target")
machine.wait_until_succeeds("pgrep -u observer plasmashell")
user = "runuser -u observer -- env XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus "
env_cmd = user + "systemctl --user show-environment"
machine.wait_until_succeeds(env_cmd + " | grep '^WAYLAND_DISPLAY='", timeout=60)
environment = dict(line.split("=", 1) for line in machine.succeed(env_cmd).splitlines() if "=" in line)
user += "WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "
run_user = user + "systemd-run --user --setenv=WAYLAND_DISPLAY=" + shlex.quote(environment["WAYLAND_DISPLAY"]) + " "
probe_unit = "capture-probe"
machine.succeed(run_user + "--unit=" + probe_unit + " sleep-capture-probe /tmp/sleep-capture.ppm")
machine.wait_until_succeeds("test -s /tmp/sleep-capture.ppm", timeout=60)
outputs = json.loads(machine.succeed(user + "kscreen-doctor -j"))["outputs"]
assert len(outputs) == 2 and all(output["connected"] for output in outputs), outputs
position = 0
for output in outputs:
    machine.succeed(user + f"kscreen-doctor output.{output['id']}.enable output.{output['id']}.position.{position},0")
    position += output["size"]["width"]
machine.succeed(run_user + "--unit=capture-client sleep-capture-client")


def frame():
    machine.succeed(user + "systemctl --user is-active " + probe_unit + ".service")
    return json.loads(machine.succeed("python /etc/capture-frame.py"))


def wait_frame(predicate, description):
    current = None
    for attempt in range(80):
        current = frame()
        if predicate(current):
            print(description, current)
            return current
        time.sleep(.25)
    print(machine.succeed(user + "journalctl --user -b -u capture-probe --no-pager | tail -40"))
    raise AssertionError(f"{description}: {current}")


def native_locked():
    return machine.succeed(user + "busctl --user --auto-start=no call org.freedesktop.ScreenSaver /ScreenSaver org.freedesktop.ScreenSaver GetActive").strip() == "b true"


def native_password(password):
    # Use real keyboard input into the greeter, including when its prompt is
    # on the virtual output. PAM gate entry independently proves acceptance.
    machine.send_monitor_command("mouse_move -10000 -10000")
    machine.send_monitor_command("mouse_move 400 300")
    machine.send_key("spc")
    time.sleep(2)
    machine.send_key("ctrl-a")
    machine.send_chars(password + "\n")


def root_method(method, token=None, *, denied=False, as_user=False):
    owner = json.loads(machine.succeed(user + "busctl --user --json=short call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus GetNameOwner s org.freedesktop.ScreenSaver"))["data"][0]
    command = (user if as_user else "") + "busctl --address=unix:path=/run/user/1000/bus --auto-start=no --allow-interactive-authorization=no --timeout=3s --json=short call " + shlex.quote(owner) + " /ScreenSaver org.kde.screensaver " + method
    if token is not None:
        command += " s " + shlex.quote(token)
    if denied:
        machine.fail(command)
        return None
    result = machine.succeed(command)
    data = json.loads(result)["data"] if result.strip() else []
    return data[0] if data else None


def current_greeter():
    pattern = r"^/nix/store/.*/libexec/\.?kscreenlocker_greet(-wrapped)?( |$)"
    command = "pgrep -o -u observer -f " + shlex.quote(pattern)
    machine.wait_until_succeeds(command)
    pid = machine.succeed(command).strip()
    assert pid.isdigit(), pid
    return pid


def no_private_frames_after(sequence):
    history = [tuple(line.split()) for line in machine.succeed("cat /tmp/sleep-capture.ppm.history").splitlines()]
    leaked = [entry for entry in history if int(entry[0]) > sequence and int(entry[2]) > 0]
    assert not leaked, leaked


sessions = json.loads(machine.succeed("loginctl list-sessions --json=short"))
session = next(s["session"] for s in sessions if s["uid"] == 1000 and s["seat"] == "seat0")
before = wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Actual private pixels captured from KWin virtual output")
machine.succeed(user + "sleep-capture-place")
machine.wait_for_text("PRIVATE DISPLAY")
machine.send_chars("BEFORE\n")
machine.wait_until_succeeds("grep -qx BEFORE /tmp/multioutput-input")

with subtest("native Plasma lock masks a real KWin virtual-output PipeWire stream"):
    machine.succeed("loginctl lock-session " + shlex.quote(session))
    for attempt in range(40):
        if native_locked():
            break
        time.sleep(.25)
    else:
        raise AssertionError("Native screen lock never acknowledged")
    masked = wait_frame(lambda f: f["sequence"] > before["sequence"] and f["private"] == 0, "New captured frame conceals private pixels")
    machine.screenshot("native-lock-physical")
    machine.succeed("chvt 63")
    time.sleep(2)
    for attempt in range(10):
        current = frame()
        assert current["sequence"] >= masked["sequence"] and current["private"] == 0, current
        assert native_locked()
        time.sleep(.25)
    machine.succeed("loginctl activate " + shlex.quote(session))
    native_password("WRONG")
    time.sleep(4)
    assert native_locked() and frame()["private"] == 0
    native_password("a")
    wait_frame(lambda f: f["sequence"] > masked["sequence"] and f["private"] > 1024 * 768 // 2, "Real native PAM restores capture outside root-managed locking")
    assert not native_locked()
    machine.wait_for_text("PRIVATE DISPLAY")
    machine.send_chars("AFTER\n")
    machine.wait_until_succeeds("grep -qx AFTER /tmp/multioutput-input")


with subtest("user bus admits its owner and root but rejects an unrelated UID"):
    query = "busctl --address=unix:path=/run/user/1000/bus --auto-start=no --allow-interactive-authorization=no --timeout=3s call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus GetId"
    machine.succeed(query)
    machine.succeed(user + query)
    runtime_mode = machine.succeed("stat -c %a /run/user/1000").strip()
    socket_mode = machine.succeed("stat -c %a /run/user/1000/bus").strip()
    try:
        # Make the socket reachable only in this disposable fixture, so denial
        # proves broker policy rather than the runtime directory's permissions.
        machine.succeed("chmod o+x /run/user/1000; chmod o+rw /run/user/1000/bus")
        connect = "import socket; s=socket.socket(socket.AF_UNIX); s.connect('/run/user/1000/bus'); s.close()"
        machine.succeed("runuser -u nobody -- python -c " + shlex.quote(connect))
        machine.fail("runuser -u nobody -- " + query)
    finally:
        machine.succeed("chmod " + socket_mode + " /run/user/1000/bus; chmod " + runtime_mode + " /run/user/1000")


with subtest("root API rejects session UID and stale lock generations"):
    token_a = root_method("LockFromRoot")
    token_b = root_method("LockFromRoot")
    assert isinstance(token_a, str) and token_a and token_b and token_a != token_b
    masked = wait_frame(lambda f: f["private"] == 0, "Root-owned native lock masks capture")
    for method, token in [("LockFromRoot", None), ("IsRootLockCurrent", token_b), ("UnlockFromRoot", token_b)]:
        root_method(method, token, denied=True, as_user=True)
    root_method("UnlockFromRoot", token_a, denied=True)
    assert root_method("IsRootLockCurrent", token_b) is True
    machine.succeed(user + "loginctl unlock-session " + shlex.quote(session))
    time.sleep(.5)
    assert native_locked() and frame()["private"] == 0
    # Ordinary Lock invalidates the token but cannot surrender managed ownership.
    machine.succeed(user + "busctl --user call org.freedesktop.ScreenSaver /ScreenSaver org.freedesktop.ScreenSaver Lock")
    machine.succeed(user + "loginctl unlock-session " + shlex.quote(session))
    root_method("UnlockFromRoot", token_b, denied=True)
    assert root_method("IsRootLockCurrent", token_b) is False
    time.sleep(.5)
    assert native_locked() and frame()["private"] == 0
    no_private_frames_after(masked["sequence"])
    token_c = root_method("LockFromRoot")
    root_method("UnlockFromRoot", token_c)
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Current root token restores capture")


with subtest("a delayed authorized greeter exit cannot complete a newer root lock"):
    token = root_method("LockFromRoot")
    masked = wait_frame(lambda f: f["private"] == 0, "Root lock precedes delayed release")
    time.sleep(2)
    old_greeter = current_greeter()
    # SIGTERM remains pending while stopped; the subsequent acquisition must
    # revoke that intent before retiring the process with SIGKILL.
    machine.succeed("kill -STOP " + old_greeter)
    root_method("UnlockFromRoot", token)
    replacement = root_method("LockFromRoot")
    assert replacement != token
    machine.wait_until_succeeds("test ! -e /proc/" + old_greeter)
    time.sleep(1)
    assert root_method("IsRootLockCurrent", replacement) is True
    assert native_locked() and frame()["private"] == 0
    root_method("UnlockFromRoot", token, denied=True)
    no_private_frames_after(masked["sequence"])
    root_method("UnlockFromRoot", replacement)
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Only replacement root authorization reveals private output")


with subtest("fresh successful native PAM cannot release a root-managed native lock"):
    token = root_method("LockFromRoot")
    masked = wait_frame(lambda f: f["private"] == 0, "Managed native lock precedes fresh native PAM")
    machine.succeed("rm -f /tmp/native-pam-entered /tmp/native-pam-finished; touch /tmp/native-pam-hold")
    time.sleep(5)
    old_greeter = current_greeter()
    native_password("a")
    machine.wait_until_succeeds("test -e /tmp/native-pam-entered", timeout=30)
    machine.succeed("rm /tmp/native-pam-hold")
    machine.wait_until_succeeds("test -e /tmp/native-pam-finished")
    machine.wait_until_succeeds("test ! -e /proc/" + old_greeter)
    time.sleep(1)
    assert native_locked() and frame()["private"] == 0
    assert root_method("IsRootLockCurrent", token) is True
    no_private_frames_after(masked["sequence"])
    root_method("UnlockFromRoot", token)
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Root release remains usable after suppressed native PAM")
    machine.wait_for_text("PRIVATE DISPLAY")


def console_locked():
    machine.wait_until_succeeds("test $(cat /sys/class/tty/tty0/active) = tty63")
    machine.wait_for_text("Unlock session")
    assert native_locked()
    current = frame()
    assert current["private"] == 0, current
    return current


def root_prompt():
    check = """
import json, subprocess
from pathlib import Path
state = json.loads(Path('/run/nixos-sleep-lock/state.json').read_text())
assert state['ready'] and state['phase'] == 'locked' and not state['holds']
supervisor = int(subprocess.check_output(['systemctl', 'show', 'nixos-sleep-lock.service', '-p', 'MainPID', '--value']))
assert supervisor > 1
def children(pid):
    return Path(f'/proc/{pid}/task/{pid}/children').read_text().split()
def command(pid):
    return Path(f'/proc/{pid}/cmdline').read_bytes().decode().strip('\\0').split('\\0')
auth = [pid for pid in children(supervisor) if command(pid)[0].endswith('/sleep-lock-native') and command(pid)[1:3] == ['authenticate', 'observer']]
assert len(auth) == 1
listeners = [pid for pid in children(auth[0]) if command(pid)[0].endswith('/plymouth') and command(pid)[1] == 'ask-for-password']
assert len(listeners) == 1
print(json.dumps([supervisor, state['epoch'], auth[0], listeners[0]]))
"""
    previous, stable = None, 0
    for attempt in range(120):
        status, output = machine.execute("python -c " + shlex.quote(check))
        current = json.loads(output) if status == 0 else None
        stable = stable + 1 if current is not None and current == previous else 0
        previous = current
        if stable >= 8:
            machine.wait_for_text("Unlock session|Authentication failed")
            return
        time.sleep(.25)
    raise AssertionError(f"No stable root PAM prompt after lock recovery: {previous}")


def console_unlock(concealed_since=None):
    root_prompt()
    if concealed_since is not None:
        no_private_frames_after(concealed_since)
    machine.send_chars("a\n")
    machine.wait_until_succeeds("test ! -e /run/nixos-sleep-lock/state.json", timeout=30)
    machine.send_key("shift")


def relock_managed_session():
    original = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
    machine.succeed("loginctl lock-session " + shlex.quote(session))
    for attempt in range(100):
        current = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
        if current["epoch"] != original["epoch"] and current["ready"]:
            console_locked()
            return
        time.sleep(.2)
    raise AssertionError(f"Native relock did not receive a fresh root authentication epoch: {current}")


with subtest("native authentication started before prepare cannot unlock after acknowledgement"):
    machine.succeed("rm -f /tmp/native-pam-entered /tmp/native-pam-finished; touch /tmp/native-pam-hold")
    machine.succeed("loginctl lock-session " + shlex.quote(session))
    time.sleep(5)
    # The prompt may be on the virtual screen. Actual PAM gate entry, after
    # requisite pam_unix succeeds, proves real keyboard authentication.
    native_password("a")
    try:
        machine.wait_until_succeeds("test -e /tmp/native-pam-entered", timeout=30)
    except Exception:
        import base64
        import os
        print("VIRTUAL_PROMPT_PNG=" + machine.succeed("python -c 'from PIL import Image; import io,base64; b=io.BytesIO(); Image.open(\"/tmp/sleep-capture.ppm\").save(b,format=\"PNG\"); print(base64.b64encode(b.getvalue()).decode())'").strip())
        print(machine.succeed("ps -eo pid,ppid,args | grep -E 'kscreenlocker|native-pam'; cat /etc/pam.d/kde"))
        machine.screenshot("native-pam-prompt")
        print("NATIVE_PROMPT_PNG=" + base64.b64encode(open(os.path.join(machine.out_dir, "native-pam-prompt.png"), "rb").read()).decode())
        raise
    machine.succeed("systemctl start systemd-suspend.service")
    console_locked()
    ack = json.loads(machine.succeed("cat /run/capture-at-backend.json"))
    supervisor = machine.succeed("systemctl show nixos-sleep-lock.service -p MainPID --value").strip()
    assert supervisor.isdigit() and int(supervisor) > 1
    machine.succeed("kill -STOP " + supervisor)
    machine.succeed("rm /tmp/native-pam-hold")
    time.sleep(3)
    history = [tuple(line.split()) for line in machine.succeed("cat /tmp/sleep-capture.ppm.history").splitlines()]
    leaked = [entry for entry in history if int(entry[0]) > ack["sequence"] and int(entry[2]) > 0]
    print("Native delayed PAM post-ack private frames:", leaked)
    machine.succeed("kill -CONT " + supervisor)
    assert not leaked, leaked
    assert native_locked()
    console_unlock(ack["sequence"])
    assert not native_locked()
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Fresh root PAM restores capture after retiring native authentication")

with subtest("root sleep boundary masks capture before acknowledgement and releases only after PAM"):
    before = frame()
    machine.succeed("rm -f /run/capture-at-backend.json; systemctl start systemd-suspend.service")
    ack = json.loads(machine.succeed("cat /run/capture-at-backend.json"))
    assert ack["sequence"] > before["sequence"] and ack["private"] == 0, ack
    console_locked()
    machine.send_chars("WRONG\n")
    time.sleep(3)
    assert frame()["private"] == 0
    machine.succeed("pkill -KILL -x plymouthd")
    console_locked()
    for attempt in range(10):
        assert native_locked() and frame()["private"] == 0
        time.sleep(.1)
    console_unlock()
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Root PAM restores virtual capture")
    assert not native_locked()
    machine.wait_for_text("PRIVATE DISPLAY")
    machine.send_chars("BRIDGE\n")
    machine.wait_until_succeeds("grep -qx BRIDGE /tmp/multioutput-input")

with subtest("one fresh root password also releases the exact preexisting native lock"):
    machine.succeed("loginctl lock-session " + shlex.quote(session))
    wait_frame(lambda f: f["private"] == 0, "Preexisting native lock conceals capture")
    machine.succeed("systemctl start systemd-suspend.service")
    console_locked()
    console_unlock()
    assert not native_locked()
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "One fresh root password restores a preexisting native lock")
    machine.wait_for_text("PRIVATE DISPLAY")

with subtest("failed greeter restart retains capture concealment while VT63 is held"):
    machine.succeed("systemctl start systemd-suspend.service")
    console_locked()
    pattern = "^/nix/store/.*/libexec/\\.?kscreenlocker_greet(-wrapped)?( |$)"
    greeter = machine.succeed("pgrep -o -u observer -f " + shlex.quote(pattern)).strip()
    assert greeter.isdigit(), greeter
    executable = machine.succeed("cat /etc/capture-greeter").strip()
    assert executable.startswith("/nix/store/") and executable.endswith("/kscreenlocker_greet"), executable
    machine.succeed("touch /run/blocked-greeter; chmod 0644 /run/blocked-greeter; mount --bind /run/blocked-greeter " + shlex.quote(executable))
    machine.succeed("kill -KILL " + greeter)
    machine.wait_until_succeeds("journalctl -b --no-pager | grep -F 'Greeter process not available; retaining the lock'")
    relock_managed_session()
    for attempt in range(20):
        assert native_locked() and frame()["private"] == 0
        assert machine.succeed("cat /sys/class/tty/tty0/active").strip() == "tty63"
        time.sleep(.2)
    machine.succeed("umount " + shlex.quote(executable))
    console_unlock()
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Root PAM recovers after failed greeter restart")
    assert not native_locked()
    machine.wait_for_text("PRIVATE DISPLAY")
    machine.send_chars("RECOVERED\n")
    machine.wait_until_succeeds("grep -qx RECOVERED /tmp/multioutput-input")

with subtest("initial greeter execution failure keeps the compositor locked"):
    machine.succeed("mount --bind /run/blocked-greeter " + shlex.quote(executable))
    machine.succeed("systemctl start systemd-suspend.service")
    console_locked()
    ack = json.loads(machine.succeed("cat /run/capture-at-backend.json"))
    assert ack["private"] == 0, ack
    machine.fail("pgrep -u observer -f " + shlex.quote(pattern))
    relock_managed_session()
    for attempt in range(10):
        assert native_locked() and frame()["private"] == 0
        time.sleep(.2)
    machine.succeed("umount " + shlex.quote(executable))
    console_unlock()
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Root PAM recovers initial greeter failure")
    assert not native_locked()

with subtest("compositor crash starts locked and fresh root PAM recovers the same session"):
    machine.succeed("systemctl start systemd-suspend.service")
    console_locked()
    original = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
    kwin_pattern = r"^/nix/store/.*/bin/\.?kwin_wayland(-wrapped)?( |$)"
    find_kwin = "pgrep -u observer -f " + shlex.quote(kwin_pattern)
    old_pid = machine.succeed(find_kwin).strip()
    assert old_pid.isdigit(), old_pid
    # A partial root password from the old compositor epoch must be discarded.
    machine.send_chars("a")
    machine.succeed("kill -KILL " + old_pid)
    machine.wait_until_succeeds("test $(" + find_kwin + ") != " + old_pid, timeout=60)
    new_pid = machine.succeed(find_kwin).strip()
    command = machine.succeed("tr '\\0' '\\n' < /proc/" + new_pid + "/cmdline").splitlines()
    assert "--lockscreen" in command, command
    for attempt in range(100):
        current = json.loads(machine.succeed("cat /run/nixos-sleep-lock/state.json"))
        if (current["epoch"] != original["epoch"] and current["ready"] and
                current["plasma"][0]["owner"] != original["plasma"][0]["owner"]):
            break
        time.sleep(.2)
    else:
        raise AssertionError(f"Same-session compositor rebind did not finish: {current}")
    assert current["plasma"][0]["id"] == original["plasma"][0]["id"]
    assert current["plasma"][0]["leader"] == original["plasma"][0]["leader"]
    assert native_locked()
    machine.succeed("rm -f /tmp/sleep-capture.ppm /tmp/sleep-capture.ppm.history")
    probe_unit = "capture-probe-reconnected"
    machine.succeed(run_user + "--unit=" + probe_unit + " sleep-capture-probe /tmp/sleep-capture.ppm")
    machine.wait_until_succeeds("test -s /tmp/sleep-capture.ppm", timeout=60)
    outputs = json.loads(machine.succeed(user + "kscreen-doctor -j"))["outputs"]
    # KWin cannot reacquire the physical card while VT63 owns DRM. Its actual
    # virtual output must still be present and concealed during that interval.
    assert any(output["connected"] and output["size"]["width"] == 1024 and output["size"]["height"] == 768 for output in outputs), outputs
    position = 0
    for output in outputs:
        machine.succeed(user + f"kscreen-doctor output.{output['id']}.enable output.{output['id']}.position.{position},0")
        position += output["size"]["width"]
    machine.succeed(run_user + "--unit=capture-client-reconnected sleep-capture-client --allow-hotplug")
    machine.succeed(user + "systemctl --user is-active capture-client-reconnected")
    console_locked()
    machine.send_chars("\n")
    time.sleep(2)
    machine.send_chars("WRONG\n")
    time.sleep(3)
    assert native_locked() and frame()["private"] == 0
    history = [tuple(line.split()) for line in machine.succeed("cat /tmp/sleep-capture.ppm.history").splitlines()]
    assert history and all(int(entry[2]) == 0 for entry in history), history
    console_unlock()
    for attempt in range(100):
        outputs = json.loads(machine.succeed(user + "kscreen-doctor -j"))["outputs"]
        if len(outputs) == 2 and all(output["connected"] for output in outputs):
            break
        time.sleep(.2)
    else:
        raise AssertionError(f"Physical output did not return after authenticated VT release: {outputs}")
    position = 0
    for output in outputs:
        machine.succeed(user + f"kscreen-doctor output.{output['id']}.enable output.{output['id']}.position.{position},0")
        position += output["size"]["width"]
    wait_frame(lambda f: f["private"] > 1024 * 768 // 2, "Fresh root PAM reveals reconnected client only after authentication")
    assert not native_locked()
    machine.succeed(user + "sleep-capture-place")
    machine.wait_for_text("PRIVATE DISPLAY", timeout=60)
    machine.send_chars("RECONNECTED\n")
    machine.wait_until_succeeds("grep -qx RECONNECTED /tmp/multioutput-input", timeout=30)
