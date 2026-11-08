"""Exercise the installed Sunshine service and helper with an isolated fixture SSH agent."""
import base64
import http.client
import json
import os
from pathlib import Path
import ssl
import subprocess
import sys
import time

home = Path("/srv/operator")
certificate = home / ".config/sunshine/server.crt"
origin = "https://sunshine.test:47990"
prefix = ["runuser", "-u", "operator", "--", "env", "XDG_RUNTIME_DIR=/run/user/1000", "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus", "DISPLAY=:0", "XAUTHORITY=/srv/operator/.Xauthority"]


def user(*args, success=True):
    result = subprocess.run(prefix + list(args), text=True, capture_output=True, timeout=60)
    assert (result.returncode == 0) == success, (args, result.stdout, result.stderr)
    return result


def request(path, data=None, cookie=None, source: str | None=origin, authorization=None):
    # A non-localhost origin exercises browser mutations outside the legacy CSRF allowlist.
    # The installed helper separately proves exact TLS pinning.
    connection = http.client.HTTPSConnection("sunshine.test", 47990, context=ssl._create_unverified_context(), timeout=5)
    headers = {}
    if source is not None:
        headers["Origin"] = source
    if cookie:
        headers["Cookie"] = cookie
    if authorization:
        headers["Authorization"] = authorization
    body = None
    if data is not None:
        headers["Content-Type"] = "application/json"
        body = json.dumps(data)
    connection.request("POST" if data is not None else "GET", path, body=body, headers=headers)
    response = connection.getresponse()
    result = response.status, dict(response.getheaders()), response.read()
    connection.close()
    return result


def start():
    user("systemctl", "--user", "start", "sunshine")
    for _ in range(60):
        try:
            if request("/ssh-login")[0] == 200:
                return
        except OSError:
            pass
        time.sleep(0.5)
    raise AssertionError(user("journalctl", "--user", "-u", "sunshine", "--no-pager").stdout)


def begin():
    code, headers, body = request("/api/ssh/begin", {})
    assert code == 200
    cookie = headers["Set-Cookie"].split(";", 1)[0]
    assert "Secure; HttpOnly; SameSite=Strict" in headers["Set-Cookie"]
    return json.loads(body)["challenge"], cookie


start()
if len(sys.argv) > 1:
    saved = json.loads(Path("/var/tmp/ssh-login-session.json").read_text())
    assert request("/api/config", cookie=saved["cookie"])[0] == 401
    assert request("/api/config", authorization="Basic " + base64.b64encode(b"admin:fixture-password").decode())[0] == 200
    print("Sunshine restart revoked SSH sessions and preserved configured password login")
    sys.exit(0)

# A brand-new key-only instance starts without any 1Password CLI or password file.
assert not (home / ".config/sunshine/credentials.json").exists()
assert Path("/etc/sunshine/ssh-keys/key-1.pub").read_text() == Path("/etc/sunshine-ssh-fixture.pub").read_text()
assert "SSH-key" in user("nixosctl-sunshine", "status").stdout
assert request("/")[0] == 307
assert request("/")[1]["Location"] == "/ssh-login"
assert request("/api/config")[0] == 401
assert request("/api/password", {"newUsername": "attacker", "newPassword": "wrong", "confirmNewPassword": "wrong"})[0] == 401
assert request("/api/ssh/begin", {}, source="https://attacker.invalid")[0] == 403
assert request("/api/ssh/begin", {}, source=None)[0] == 403

user("install", "-m600", "/etc/sunshine-ssh-fixture", str(home / "fixture-key"))
user("install", "-m644", "/etc/sunshine-ssh-fixture.pub", str(home / "fixture-key.pub"))
sock = "/run/user/1000/sunshine-test-agent.sock"
agent = subprocess.Popen(prefix + ["ssh-agent", "-D", "-a", sock], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
try:
    for _ in range(100):
        if Path(sock).exists():
            break
        time.sleep(0.05)
    user("env", "SSH_AUTH_SOCK=" + sock, "ssh-add", str(home / "fixture-key"))
    user("rm", str(home / "fixture-key"))

    def approve(challenge):
        user("env", "SSH_AUTH_SOCK=" + sock, "sunshine-ssh-login", origin, "--challenge", challenge, "-i", str(home / "fixture-key.pub"), "--certificate", str(certificate))

    challenge, binding = begin()
    assert request("/api/ssh/exchange", {"challenge": challenge}, cookie=binding)[0] == 202
    approve(challenge)
    assert request("/api/ssh/exchange", {"challenge": challenge})[0] == 202
    code, headers, _ = request("/api/ssh/exchange", {"challenge": challenge}, cookie=binding)
    assert code == 200
    cookie = headers["Set-Cookie"].split(";", 1)[0]
    assert "Secure; HttpOnly; SameSite=Strict" in headers["Set-Cookie"]
    assert request("/api/ssh/exchange", {"challenge": challenge}, cookie=binding)[0] == 202
    assert request("/api/config", cookie=cookie)[0] == 200
    assert request("/api/csrf-token", cookie=cookie)[0] == 200
    assert request("/api/password", {}, cookie=cookie, source="https://attacker.invalid")[0] == 401
    assert request("/api/ssh/logout", {}, cookie=cookie, source="https://attacker.invalid")[0] == 403
    assert request("/api/ssh/logout", {}, cookie=cookie)[0] == 200
    assert request("/api/config", cookie=cookie)[0] == 401

    challenge, binding = begin()
    approve(challenge)
    code, headers, _ = request("/api/ssh/exchange", {"challenge": challenge}, cookie=binding)
    assert code == 200
    cookie = headers["Set-Cookie"].split(";", 1)[0]
    code, _, body = request("/api/password", {"newUsername": "admin", "newPassword": "fixture-password", "confirmNewPassword": "fixture-password"}, cookie=cookie)
    assert code == 200 and json.loads(body)["status"] is True
    assert request("/api/config", cookie=cookie)[0] == 401
    assert request("/api/config", authorization="Basic " + base64.b64encode(b"admin:fixture-password").decode())[0] == 200
    challenge, binding = begin()
    approve(challenge)
    code, headers, _ = request("/api/ssh/exchange", {"challenge": challenge}, cookie=binding)
    assert code == 200
    cookie = headers["Set-Cookie"].split(";", 1)[0]
    Path("/var/tmp/ssh-login-session.json").write_text(json.dumps({"cookie": cookie}))
    assert request("/api/config", cookie=cookie)[0] == 200
    # Existing credentials survive repeated preparation and the legacy-file transition.
    user("systemctl", "--user", "stop", "sunshine")
    credentials = home / ".config/sunshine/credentials.json"
    password_record = json.loads(credentials.read_text())
    assert credentials.stat().st_mode & 0o777 == 0o600
    user("nixosctl-sunshine", "prepare")
    assert json.loads(credentials.read_text()) == password_record
    state = home / ".config/sunshine/sunshine_state.json"
    legacy = json.loads(state.read_text()) if state.exists() else {}
    legacy.update(password_record)
    state.write_text(json.dumps(legacy))
    state.chmod(0o600)
    os.chown(state, 1000, 100)
    credentials.unlink()
    user("nixosctl-sunshine", "prepare")
    assert json.loads(credentials.read_text()) == password_record
    assert json.loads(state.read_text()) == legacy
    print("Installed SSH helper, agent signing, browser binding, key-only setup, CSRF, logout, and password revocation passed")
finally:
    agent.terminate()
    agent.wait(timeout=5)
