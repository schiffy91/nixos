"""Root-only sleep ordering; passwords stay in the native PAM conversation."""
import contextlib
import fcntl
import json
import os
from pathlib import Path
import pwd
import secrets
import socket
import stat
import subprocess
import sys
import time

DIRECTORY = Path("/run/nixos-sleep-lock")
STATE = DIRECTORY / "state.json"
NATIVE = "@native@"
BLANK = "@blank@"
PLYMOUTH = "@plymouth@"
PLYMOUTHD = "@plymouthd@"
SYSTEMCTL = "@systemctl@"
LOGINCTL = "@loginctl@"
USER = "@user@"
UNIT = "nixos-sleep-lock.service"
GAMING_MARKER = Path("/run/nixos-gaming-locked")
SUNSHINE_UNITS = ("sunshine.service", "app-dev.lizardbyte.app.Sunshine.service")
SLEEP_UNITS = {
    "systemd-suspend.service", "systemd-hibernate.service",
    "systemd-hybrid-sleep.service", "systemd-suspend-then-hibernate.service",
}


def command(*args, check=True, timeout=15):
    return subprocess.run(args, check=check, timeout=timeout,
                          stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL)


def private(path, mode, directory=False):
    info = path.lstat()
    kind = stat.S_ISDIR if directory else stat.S_ISREG
    if not kind(info.st_mode) or info.st_uid != 0 or stat.S_IMODE(info.st_mode) != mode:
        raise RuntimeError("Unsafe sleep-lock state permissions")
    if not directory and info.st_nlink != 1:
        raise RuntimeError("Sleep-lock state has multiple links")


@contextlib.contextmanager
def transaction():
    # /run is root-owned. Only root can create or replace this private leaf.
    DIRECTORY.mkdir(mode=0o700, exist_ok=True)
    private(DIRECTORY, 0o700, directory=True)
    fd = os.open(DIRECTORY / "mutex", os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
    try:
        private(DIRECTORY / "mutex", 0o600)
        fcntl.flock(fd, fcntl.LOCK_EX)
        yield
    finally:
        os.close(fd)


def load():
    if not STATE.exists() and not STATE.is_symlink():
        return None
    private(STATE, 0o600)
    data = json.loads(STATE.read_text())
    if (not isinstance(data, dict)
            or set(data) != {"uid", "vt", "sysrq", "epoch", "holds", "phase", "ready", "sunshine"}
            or type(data["uid"]) is not int or data["uid"] < 0
            or (data["sunshine"] is not None and
                (not isinstance(data["sunshine"], list) or
                 not all(unit in SUNSHINE_UNITS for unit in data["sunshine"])))
            or type(data["ready"]) is not bool
            or type(data["vt"]) is not int or not 0 < data["vt"] < 63
            or type(data["sysrq"]) is not int or data["sysrq"] < 0
            or not isinstance(data["epoch"], str) or len(data["epoch"]) != 32
            or not isinstance(data["holds"], list)
            or not all(isinstance(hold, str) for hold in data["holds"])
            or not set(data["holds"]) <= SLEEP_UNITS
            or not isinstance(data["phase"], str)
            or data["phase"] not in {"acquiring", "locked", "authenticated"}):
        raise RuntimeError("Invalid sleep-lock state")
    if data["uid"] != pwd.getpwnam(USER).pw_uid:
        raise RuntimeError("The configured password account changed")
    return data


def save(data):
    path = DIRECTORY / (".state-" + secrets.token_hex(16))
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        with os.fdopen(fd, "w") as file:
            json.dump(data, file)
            file.flush()
            os.fsync(file.fileno())
        os.replace(path, STATE)
    finally:
        path.unlink(missing_ok=True)


def gaming_session():
    def loginctl(*args):
        return subprocess.run([LOGINCTL, "--no-ask-password", *args], check=True,
                              timeout=3, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, env={"LANG": "C.UTF-8"}, text=True).stdout

    uid = pwd.getpwnam(USER).pw_uid
    for entry in json.loads(loginctl("list-sessions", "--json=short")):
        if entry["class"] != "user":
            continue
        output = loginctl("show-session", entry["session"], "--property=User",
                          "--property=Type", "--property=Desktop")
        state = dict(line.split("=", 1) for line in output.splitlines())
        if (int(state["User"]) == uid and state["Type"] == "wayland" and
                "gamescope" in state["Desktop"].lower().split(":")):
            return True
    return False


def user_service(*args):
    account = pwd.getpwnam(USER)
    return subprocess.run([SYSTEMCTL, "--user", "--no-ask-password", *args],
                          user=account.pw_uid, group=account.pw_gid, extra_groups=[],
                          env={"XDG_RUNTIME_DIR": f"/run/user/{account.pw_uid}",
                               "DBUS_SESSION_BUS_ADDRESS": f"unix:path=/run/user/{account.pw_uid}/bus"},
                          stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True, timeout=10, check=True).stdout.strip()


def stream_state(unit):
    output = user_service("show", unit, "-p", "LoadState,ActiveState,MainPID")
    state = dict(line.split("=", 1) for line in output.splitlines())
    if state.get("LoadState") == "not-found":
        return None
    if (not state.get("ActiveState") or not state.get("MainPID", "").isdigit()):
        raise RuntimeError("Cannot determine Sunshine service state")
    return state


def stop_streams(data):
    fd = os.open(GAMING_MARKER, os.O_WRONLY | os.O_CREAT | os.O_NOFOLLOW | os.O_CLOEXEC, 0o644)
    os.fchmod(fd, 0o644)
    os.close(fd)
    private(GAMING_MARKER, 0o644)
    states = {unit: stream_state(unit) for unit in SUNSHINE_UNITS}
    if data["sunshine"] is None:
        # Save the original state before the first stop, including crash recovery.
        data["sunshine"] = [unit for unit, state in states.items()
                            if state and state["ActiveState"] not in {"inactive", "failed"}]
        save(data)
    for unit, state in states.items():
        if state and (state["ActiveState"] not in {"inactive", "failed"} or state["MainPID"] != "0"):
            user_service("stop", unit)
    verify_streams()


def verify_streams():
    private(GAMING_MARKER, 0o644)
    for unit in SUNSHINE_UNITS:
        state = stream_state(unit)
        if state and (state["ActiveState"] not in {"inactive", "failed"} or state["MainPID"] != "0"):
            raise RuntimeError("A Sunshine service is still running while Gaming Mode is locked")


def restore_streams(data):
    GAMING_MARKER.unlink(missing_ok=True)
    for unit in data["sunshine"] or []:
        try:
            user_service("start", unit)
        except subprocess.SubprocessError:
            # A failed stream restart must not invalidate completed password auth.
            print("Could not restart Sunshine after unlocking", file=sys.stderr)


def finish(data):
    if data["phase"] != "authenticated" or data["holds"]:
        raise RuntimeError("Unlock has not been authenticated")
    command(NATIVE, "release", str(data["vt"]))
    Path("/proc/sys/kernel/sysrq").write_text(str(data["sysrq"]))
    restore_streams(data)
    STATE.unlink()


def recover_release(data):
    # Never replay an interrupted release after restart. Reacquire the console
    # and require fresh PAM success before returning to the game.
    data["phase"] = "acquiring"
    data["ready"] = False
    data["epoch"] = secrets.token_hex(16)
    save(data)


def reset_authentication(data):
    # Every console reacquisition invalidates an earlier password attempt.
    data["epoch"] = secrets.token_hex(16)
    data["ready"] = False
    save(data)


def prepare(unit):
    with transaction():
        data = load()
        if data and data["phase"] == "authenticated":
            recover_release(data)
        if data is None and not gaming_session():
            return
        if data is None:
            vt = Path("/sys/class/tty/tty0/active").read_text().strip()
            if not vt.startswith("tty") or not vt[3:].isdigit() or not 0 < int(vt[3:]) < 63:
                raise RuntimeError("Cannot identify the session console")
            data = {"uid": pwd.getpwnam(USER).pw_uid,
                    "vt": int(vt[3:]), "sysrq": int(Path("/proc/sys/kernel/sysrq").read_text()),
                    "epoch": "", "holds": [], "phase": "acquiring", "ready": False, "sunshine": None}
        data["epoch"] = secrets.token_hex(16)
        data["ready"] = False
        data["holds"] = sorted(set(data["holds"]) | {unit})
        already_locked = data["phase"] == "locked"
        save(data)
        try:
            stop_streams(data)
            reset_authentication(data)
        finally:
            # Stream shutdown failure must still conceal the physical session and
            # deny the sleep backend. Recovery retries on the held console.
            Path("/proc/sys/kernel/sysrq").write_text("0")
            command(NATIVE, "verify" if data["phase"] == "locked" else "secure")
            data["phase"] = "locked"
            save(data)
            if not already_locked:
                command(BLANK)
            # An earlier request's Plymouth may still own DRM. Its cycle must
            # observe the new epoch and exit before the next cycle pre-blanks.
            # No backend runs until that cycle publishes fresh readiness.
    command(SYSTEMCTL, "start", UNIT, timeout=30)
    for _ in range(200):
        with transaction():
            data = load()
            if not data or data["phase"] != "locked" or unit not in data["holds"]:
                raise RuntimeError("Sleep lock lost its pending request")
            command(NATIVE, "verify")
            ready = data["ready"]
        if ready:
            verify_streams()
            command(PLYMOUTH, "--has-safe-displays")
            command(SYSTEMCTL, "is-active", "--quiet", UNIT)
            return
        time.sleep(.1)
    raise RuntimeError("Password display was not acknowledged")


def resume(unit):
    with transaction():
        data = load()
        if data:
            data["holds"] = [hold for hold in data["holds"] if hold != unit]
            save(data)
    if data:
        # ExecStartPre can die after recording acquisition intent, before it
        # starts the controller. Finish that intent even when sleep is aborted.
        command(SYSTEMCTL, "start", "--no-block", UNIT)


def notify_ready():
    path = os.environ["NOTIFY_SOCKET"]
    if path.startswith("@"):
        path = "\0" + path[1:]
    with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as sock:
        sock.connect(path)
        sock.sendall(b"READY=1")


def cycle():
    with transaction():
        data = load()
        if not data:
            return
        if data["phase"] == "authenticated":
            recover_release(data)
        if data["phase"] == "acquiring":
            # Recover interrupted preparation by completing the boundary. Never
            # release a console merely because the sleep request was cancelled.
            Path("/proc/sys/kernel/sysrq").write_text("0")
            command(NATIVE, "secure")
            data["phase"] = "locked"
        command(NATIVE, "verify")
        data["ready"] = False
        epoch, pending_seen = data["epoch"], bool(data["holds"])
        save(data)
    command(BLANK)
    with transaction():
        data = load()
        if not data or data["phase"] != "locked" or data["epoch"] != epoch:
            return
        stop_streams(data)
        reset_authentication(data)
        epoch = data["epoch"]
    daemon = subprocess.Popen([PLYMOUTHD, "--no-daemon", "--no-boot-log", "--tty=/dev/tty63",
                               "--mode=boot-up", "--pid-file=" + str(DIRECTORY / "plymouth.pid")],
                              stdin=subprocess.DEVNULL)
    try:
        for _ in range(100):
            if daemon.poll() is not None:
                raise RuntimeError("Password display exited")
            if command(PLYMOUTH, "--ping", check=False).returncode == 0:
                break
            time.sleep(.1)
        else:
            raise RuntimeError("Password display did not start")
        command(PLYMOUTH, "show-splash")
        for _ in range(100):
            if daemon.poll() is not None:
                raise RuntimeError("Password display exited")
            if command(PLYMOUTH, "--has-safe-displays", check=False).returncode == 0:
                break
            time.sleep(.1)
        else:
            raise RuntimeError("Not every display is safely covered")
        with transaction():
            data = load()
            if not data or data["phase"] != "locked":
                raise RuntimeError("Session lock state changed unexpectedly")
            if data["epoch"] != epoch:
                return
            verify_streams()
            data["ready"] = True
            save(data)
        retry = False
        while True:
            if daemon.poll() is not None:
                raise RuntimeError("Password display exited")
            command(PLYMOUTH, "--has-safe-displays")
            with transaction():
                data = load()
                if not data or data["phase"] != "locked":
                    raise RuntimeError("Session lock state changed unexpectedly")
                command(NATIVE, "verify")
                if data["epoch"] != epoch:
                    return
                pending = bool(data["holds"])
                if not pending and pending_seen:
                    # Suspend can invalidate the renderer's scanout buffers.
                    # Recreate the UI after the backend ends, retaining the VT
                    # lock throughout and discarding pre-sleep input/cache.
                    data["ready"] = False
                    save(data)
                    return
            if pending:
                pending_seen = True
                time.sleep(.1)
                continue
            # systemd-sleep freezes user.slice, including the user bus. Once
            # acknowledged, do not wait on that frozen bus during the backend.
            verify_streams()
            # Keep supervising the request generation while PAM waits for input.
            # A later sleep must cancel this prompt before acknowledging its UI.
            authentication = subprocess.Popen(
                [NATIVE, "authenticate", USER, *(["retry"] if retry else [])],
                stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
            try:
                while authentication.poll() is None:
                    if daemon.poll() is not None:
                        raise RuntimeError("Password display exited")
                    command(PLYMOUTH, "--has-safe-displays")
                    with transaction():
                        current = load()
                        if not current or current["phase"] != "locked":
                            raise RuntimeError("Session lock state changed unexpectedly")
                        if current["epoch"] != epoch or current["holds"]:
                            return
                        verify_streams()
                    time.sleep(.1)
            finally:
                if authentication.poll() is None:
                    authentication.terminate()
                    try:
                        authentication.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        authentication.kill()
                        authentication.wait()
            if authentication.returncode != 0:
                retry = True
                continue
            with transaction():
                data = load()
                if not data or data["phase"] != "locked":
                    raise RuntimeError("Session lock state changed unexpectedly")
                if data["epoch"] != epoch or data["holds"]:
                    retry = False
                    continue
                command(NATIVE, "verify")
                command(PLYMOUTH, "quit")
                daemon.wait(timeout=10)
                data["phase"] = "authenticated"
                save(data)
                finish(data)
                return
    finally:
        # UI cleanup never releases the console lock.
        if daemon.poll() is None:
            daemon.terminate()
            try:
                daemon.wait(timeout=5)
            except subprocess.TimeoutExpired:
                daemon.kill()
                daemon.wait()


def run():
    # Stay alive between lock cycles. A new request can arrive immediately
    # after release without racing systemd's previous service shutdown.
    with transaction():
        data = load()
        if data:
            data["ready"] = False
            save(data)
    notify_ready()
    while True:
        cycle()
        time.sleep(.1)


def main():
    if os.getuid() != 0 or os.geteuid() != 0:
        raise RuntimeError("Session locking requires root")
    os.umask(0o077)
    if len(sys.argv) == 2 and sys.argv[1] == "run":
        run()
    elif len(sys.argv) == 3 and sys.argv[1] in {"prepare", "resume"} and sys.argv[2] in SLEEP_UNITS:
        (prepare if sys.argv[1] == "prepare" else resume)(sys.argv[2])
    else:
        raise RuntimeError("Invalid sleep-lock operation")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print("Sleep lock failed:", error, file=sys.stderr)
        sys.exit(1)
