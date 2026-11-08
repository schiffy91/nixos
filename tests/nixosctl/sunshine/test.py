import base64
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import socket
import ssl
import subprocess
import sys
import time
import urllib.error
import urllib.request

prefix = ["runuser", "-u", "operator", "--", "env", "XDG_RUNTIME_DIR=/run/user/1000", "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus", "DISPLAY=:0", "XAUTHORITY=/srv/operator/.Xauthority"]
directory = Path("/srv/operator/.config/sunshine")
credentials = directory / "credentials.json"
state = directory / "sunshine_state.json"


def user(*args, data=None, success=True):
    result = subprocess.run(prefix + list(args), input=data, text=True, capture_output=True, timeout=90)
    assert (result.returncode == 0) == success, (args, result.stdout, result.stderr)
    return result


def provision(password="first-password"):
    return user("nixosctl", "sunshine", "provision", data=json.dumps({"username": "admin", "password": password}))


def api(password="first-password", endpoint="/api/config", payload=None):
    headers = {"Authorization": "Basic " + base64.b64encode(("admin:" + password).encode()).decode()}
    data = None if payload is None else json.dumps(payload).encode()
    if data is not None:
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request("https://localhost:47990" + endpoint, data=data, headers=headers)
    try:
        with urllib.request.urlopen(request, context=ssl._create_unverified_context(), timeout=4) as response:
            return response.status, json.loads(response.read())
    except urllib.error.HTTPError as error:
        return error.code, None


def start(password="first-password"):
    user("systemctl", "--user", "reset-failed")
    user("systemctl", "--user", "start", "sunshine")
    for _ in range(40):
        try:
            if api(password)[0] == 200:
                return
        except (OSError, urllib.error.URLError):
            pass
        time.sleep(0.5)
    raise AssertionError(user("journalctl", "--user", "-u", "sunshine", "--no-pager").stdout)


def no_port():
    with socket.socket() as probe:
        assert probe.connect_ex(("127.0.0.1", 47990)) != 0


if len(sys.argv) > 1:
    # No 1Password CLI exists in this guest. Cached authentication must work offline.
    start("updated-password")
    assert api("first-password")[0] == 401
    assert json.loads(state.read_text())["root"] == json.loads(Path("/var/tmp/pairing.json").read_text())
    print("offline authenticated restart and pairing persistence passed")
    sys.exit(0)

user("systemctl", "--user", "start", "sunshine", success=False)
no_port()
# Direct launches also fail closed, even without the service prepare step.
rejected = user("sunshine", "capture=x11", "encoder=software", "system_tray=disabled", success=False)
assert "credentials are missing or invalid" in rejected.stdout + rejected.stderr
no_port()

# Import the old authentication fields without changing any legacy state.
user("mkdir", "-p", str(directory))
salt = "0123456789abcdef"
legacy = {"username": "admin", "salt": salt, "password": hashlib.sha256(("first-password" + salt).encode()).digest()[::-1].hex().upper()}
state.write_text(json.dumps(legacy))
os.chown(state, 1000, 100)
state.chmod(0o600)
original = state.read_bytes()
user("nixosctl-sunshine", "prepare")
assert state.read_bytes() == original
assert json.loads(credentials.read_text()) == legacy
start()
assert api("wrong-password")[0] == 401

# Pair a real Moonlight client, then preserve that state during credential rotation.
with open("/var/tmp/moonlight-pair.log", "w") as log:
    pairing = subprocess.Popen(prefix + ["moonlight", "pair", "localhost", "--pin", "1234"], stdout=log, stderr=log)
    try:
        pending = []
        for _ in range(40):
            code, response = api(endpoint="/api/pin")
            assert code == 200 and response is not None
            pending = response.get("pairings", [])
            if pending:
                break
            time.sleep(0.5)
        assert pending, Path("/var/tmp/moonlight-pair.log").read_text()
        code, result = api(endpoint="/api/pin", payload={"pairing_id": pending[0]["id"], "pin": "1234", "name": "test-client"})
        assert code == 200 and result is not None and result["status"] is True
        for _ in range(60):
            if state.exists() and json.loads(state.read_text()).get("root", {}).get("named_devices"):
                break
            time.sleep(0.5)
        paired = json.loads(state.read_text()).get("root", {}).get("named_devices", [])
        assert paired and "BEGIN CERTIFICATE" in paired[0]["cert"], Path("/var/tmp/moonlight-pair.log").read_text()
    finally:
        if pairing.poll() is None:
            pairing.terminate()
            pairing.wait(timeout=5)
user("systemctl", "--user", "stop", "sunshine")
paired_state = state.read_bytes()
paired_root = json.loads(paired_state)["root"]
Path("/var/tmp/pairing.json").write_text(json.dumps(paired_root))

with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
    list(executor.map(provision, ["updated-password"] * 8))
assert state.read_bytes() == paired_state
protected = credentials.read_bytes()
provision("updated-password")
assert credentials.read_bytes() == protected
assert credentials.stat().st_uid == 1000 and credentials.stat().st_mode & 0o777 == 0o600
assert directory.stat().st_mode & 0o777 == 0o700
user("nixosctl", "sunshine", "sync", success=False)
assert credentials.read_bytes() == protected  # Failed vault retrieval cannot clobber offline auth.

for kind in ("symlink", "hardlink", "public", "owner", "malformed"):
    credentials.unlink()
    credentials.write_bytes(protected)
    os.chown(credentials, 1000, 100)
    credentials.chmod(0o600)
    if kind == "symlink":
        credentials.unlink()
        credentials.symlink_to(state)
    if kind == "hardlink":
        os.link(credentials, directory / "alias")
    if kind == "public": credentials.chmod(0o644)
    if kind == "owner": os.chown(credentials, 0, 0)
    if kind == "malformed": credentials.write_text("{invalid")
    user("nixosctl", "sunshine", "provision", data=json.dumps({"username": "admin", "password": "never-used"}), success=False)
    assert state.read_bytes() == paired_state
    if kind == "hardlink": (directory / "alias").unlink()
credentials.unlink()
credentials.write_text(json.dumps({"username": "", "password": "", "salt": ""}))
os.chown(credentials, 1000, 100)
credentials.chmod(0o600)
rejected = user("sunshine", "capture=x11", "encoder=software", "system_tray=disabled", success=False)
assert "credentials are missing or invalid" in rejected.stdout + rejected.stderr
no_port()
credentials.write_bytes(protected)
start("updated-password")
assert api("first-password")[0] == 401
assert json.loads(state.read_text())["root"] == paired_root
print("missing credentials, actual HTTP authentication, Moonlight pairing, atomic updates and unsafe files passed")
