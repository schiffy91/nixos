"""Keep Plasma's virtual outputs locked while the root console owns input.

These are compositor state checks, not an encoded-frame acknowledgement. The
capture regression must establish what the pinned KWin renders in that state.
"""
import json
import pwd
import re
import subprocess
import time

LOGINCTL = "@loginctl@"
BUSCTL = "@busctl@"


def run(*args, uid=None):
    credentials = {}
    if uid is not None:
        account = pwd.getpwuid(uid)
        # Connect as the bus owner without a login/PAM or authentication agent.
        credentials = {"user": uid, "group": account.pw_gid, "extra_groups": []}
    return subprocess.run(args, check=True, timeout=3, stdin=subprocess.DEVNULL,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          env={"LANG": "C.UTF-8"}, text=True, **credentials).stdout


def session(identifier):
    text = run(LOGINCTL, "--no-ask-password", "show-session", identifier,
               "--property=Id", "--property=User", "--property=Desktop",
               "--property=Type", "--property=Class", "--property=Leader",
               "--property=TimestampMonotonic", "--property=LockedHint")
    return dict(line.split("=", 1) for line in text.splitlines())


def graphical_sessions():
    entries = json.loads(run(LOGINCTL, "--no-ask-password", "list-sessions", "--json=short"))
    result = []
    for entry in entries:
        if entry["class"] != "user":
            continue
        state = session(entry["session"])
        if state["Type"] in {"wayland", "x11"}:
            result.append(state)
    return result


def is_plasma(state):
    return bool({"kde", "plasma"} & set(state["Desktop"].lower().split(":")))


def plasma_sessions():
    result = [state for state in graphical_sessions() if is_plasma(state)]
    if any(state["Type"] != "wayland" for state in result):
        raise RuntimeError("Sleep locking supports Plasma Wayland only")
    return result


def bus(uid, destination, path, interface, method, *args, void=False, root=False):
    output = run(BUSCTL, "--auto-start=no", "--expect-reply=yes",
                 "--allow-interactive-authorization=no", "--timeout=2s", "--json=short",
                 f"--address=unix:path=/run/user/{uid}/bus", "call",
                 destination, path, interface, method, *args, uid=None if root else uid)
    if void:
        if output:
            raise RuntimeError("Unexpected Plasma lock response")
        return
    answer = json.loads(output)
    if len(answer["data"]) != 1:
        raise RuntimeError("Unexpected Plasma bus response")
    return answer["data"][0]


def owner(uid):
    name = bus(uid, "org.freedesktop.DBus", "/org/freedesktop/DBus",
               "org.freedesktop.DBus", "GetNameOwner", "s", "org.freedesktop.ScreenSaver")
    if not isinstance(name, str) or re.fullmatch(r":[0-9]+\.[0-9]+", name) is None:
        raise RuntimeError("Invalid Plasma bus owner")
    return name


def bus_id(uid):
    value = bus(uid, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetId")
    if not isinstance(value, str) or re.fullmatch(r"[0-9a-f]{32}", value) is None:
        raise RuntimeError("Invalid Plasma bus identity")
    return value


def active(record):
    value = bus(record["uid"], record["owner"], "/ScreenSaver",
                "org.freedesktop.ScreenSaver", "GetActive")
    if type(value) is not bool:
        raise RuntimeError("Invalid Plasma lock state")
    return value


def root_call(record, method, *args, void=False):
    return bus(record["uid"], record["owner"], "/ScreenSaver", "org.kde.screensaver",
               method, *args, void=void, root=True)


def token_valid(token):
    return (isinstance(token, str) and
            re.fullmatch(r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}", token) is not None)


def capture(username):
    uid = pwd.getpwnam(username).pw_uid
    result = []
    for state in plasma_sessions():
        if int(state["User"]) != uid:
            raise RuntimeError("Another account has a Plasma session; refuse to unlock it")
        if result:
            raise RuntimeError("Multiple Plasma sessions share a user bus")
        record = {"id": state["Id"], "uid": uid, "leader": int(state["Leader"]),
                  "started": state["TimestampMonotonic"], "bus": bus_id(uid),
                  "owner": owner(uid), "token": ""}
        result.append(record)
    return result


def valid(records):
    return (isinstance(records, list) and len(records) <= 1 and all(
        isinstance(record, dict) and
        set(record) == {"id", "uid", "leader", "started", "bus", "owner", "token"} and
        isinstance(record["id"], str) and re.fullmatch(r"[a-zA-Z0-9_-]+", record["id"]) is not None and
        type(record["uid"]) is int and record["uid"] > 0 and
        type(record["leader"]) is int and record["leader"] > 0 and
        isinstance(record["started"], str) and record["started"].isdigit() and
        isinstance(record["bus"], str) and re.fullmatch(r"[0-9a-f]{32}", record["bus"]) is not None and
        isinstance(record["owner"], str) and re.fullmatch(r":[0-9]+\.[0-9]+", record["owner"]) is not None and
        (record["token"] == "" or token_valid(record["token"])) for record in records))


def session_identity(record):
    state = session(record["id"])
    if (state["Type"] != "wayland" or state["Class"] != "user" or not is_plasma(state) or
            state["Id"] != record["id"] or int(state["User"]) != record["uid"] or
            int(state["Leader"]) != record["leader"] or state["TimestampMonotonic"] != record["started"]):
        raise RuntimeError("The recorded Plasma session changed")
    return state


def identity(record):
    state = session_identity(record)
    if (bus_id(record["uid"]) != record["bus"] or
            owner(record["uid"]) != record["owner"]):
        raise RuntimeError("The recorded Plasma lock owner changed")
    return state


def inventory(records):
    if not valid(records):
        raise RuntimeError("Invalid recorded Plasma sessions")
    if {state["Id"] for state in plasma_sessions()} != {record["id"] for record in records}:
        raise RuntimeError("The Plasma session inventory changed")


def rebind(records):
    # A restarted compositor may get a new bus owner within the same logind
    # session. Its caller must invalidate the root authentication generation
    # before admitting these refreshed records and requesting a new lock.
    inventory(records)
    result = []
    for record in records:
        session_identity(record)
        current = dict(record, bus=bus_id(record["uid"]), owner=owner(record["uid"]))
        identity(current)
        result.append(current)
    inventory(result)
    return result


def verify(records):
    inventory(records)
    for record in records:
        state = identity(record)
        current = root_call(record, "IsRootLockCurrent", "s", record["token"])
        if type(current) is not bool:
            raise RuntimeError("Invalid native root lock state")
        if not current or state["LockedHint"] != "yes" or not active(record):
            raise RuntimeError("Plasma has not kept its session locked")


def lock(records):
    if not valid(records):
        raise RuntimeError("Invalid recorded Plasma sessions")
    locked = []
    for record in records:
        identity(record)
        # The synchronous root-only method returns new native authority after
        # invalidating old greeter results and all prior release tokens.
        token = root_call(record, "LockFromRoot")
        if not token_valid(token):
            raise RuntimeError("Invalid native root lock token")
        locked.append(dict(record, token=token))
    deadline = time.monotonic() + 8
    while True:
        try:
            verify(locked)
            return locked
        except RuntimeError:
            if time.monotonic() >= deadline:
                raise
            time.sleep(.1)


def release(records):
    inventory(records)
    for record in records:
        identity(record)
        if not active(record):
            identity(record)
            continue
        # A forwarded logind signal has no generation and also permits the
        # session UID. Use only this current native token over a root connection.
        root_call(record, "UnlockFromRoot", "s", record["token"], void=True)
        deadline = time.monotonic() + 8
        while active(record):
            identity(record)
            if time.monotonic() >= deadline:
                raise RuntimeError("Plasma did not release its authenticated lock")
            time.sleep(.1)
        identity(record)
    inventory(records)
