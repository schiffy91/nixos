"""Black-box contracts for the installed BTRC gaming helpers; no desktop needed."""
import concurrent.futures
import hashlib
import http.server
import json
import os
from pathlib import Path
import pwd
import signal
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import time
import xml.etree.ElementTree as ET

TOOLS, SSH, ARTWORK = map(Path, sys.argv[1:])
ROOT = Path(tempfile.mkdtemp(prefix="gaming-runtime-", dir=Path.cwd()))


def run(*args, data=None, success=True, env=None):
    result = subprocess.run(list(map(str, args)), input=data, text=True, capture_output=True, env=env, timeout=20)
    assert (result.returncode == 0) == success, (args, result.returncode, result.stdout, result.stderr)
    return result


def wait(condition):
    for _ in range(150):
        if condition():
            return
        time.sleep(0.05)
    raise AssertionError("helper did not reach the expected state")


def credentials():
    home = ROOT / "home"
    home.mkdir(mode=0o700)
    config = ROOT / "credentials.json"
    config.write_text(json.dumps({"user": pwd.getpwuid(os.getuid()).pw_name, "home": str(home), "publicKeys": [], "usernameReference": "op://missing/user", "passwordReference": "op://missing/password"}))
    command = [TOOLS / "credentials", config]
    run(*command, "prepare", success=False)
    directory = home / ".config/sunshine"
    target = directory / "credentials.json"
    legacy = directory / "sunshine_state.json"
    salt = "0123456789abcdef"
    record = {"username": "admin", "salt": salt, "password": hashlib.sha256(("first-password" + salt).encode()).digest()[::-1].hex().upper()}
    legacy.write_text(json.dumps({**record, "root": {"paired": ["client"]}}))
    legacy.chmod(0o644)
    original = legacy.read_bytes()
    run(*command, "prepare")
    assert json.loads(target.read_text()) == record and legacy.read_bytes() == original
    payload = json.dumps({"username": "admin", "password": "updated-passé🔑"})
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(lambda _: run(*command, "provision", data=payload), range(8)))
    current = target.read_bytes()
    saved = json.loads(current)
    assert saved["password"] == hashlib.sha256(("updated-passé🔑" + saved["salt"]).encode()).digest()[::-1].hex().upper()
    assert target.stat().st_mode & 0o777 == 0o600
    assert directory.stat().st_mode & 0o777 == 0o700
    run(*command, "provision", data=payload)
    assert target.read_bytes() == current and legacy.read_bytes() == original
    run(*command, "sync", success=False)
    assert target.read_bytes() == current
    for data in ["{invalid", '{"username":"a:b","password":"x"}', '{"username":"admin","password":"a\\u0000b"}', '{"username":"","password":"x"}', '{"username":"admin","password":""}', " " * 32769]:
        run(*command, "provision", data=data, success=False)
        assert target.read_bytes() == current
    for password in ["\ud800", "\udc00", "\ud800x"]:
        run(*command, "provision", data=json.dumps({"username": "admin", "password": password}), success=False)
        assert target.read_bytes() == current
    for kind in ["symlink", "hardlink", "public", "fifo", "malformed"]:
        target.unlink()
        if kind == "symlink":
            target.symlink_to(legacy)
        elif kind == "fifo":
            os.mkfifo(target, 0o600)
        else:
            target.write_bytes(b"{invalid" if kind == "malformed" else current)
            target.chmod(0o644 if kind == "public" else 0o600)
            if kind == "hardlink":
                os.link(target, directory / "alias")
        run(*command, "provision", data=payload, success=False)
        assert legacy.read_bytes() == original
        target.unlink()
        if kind == "hardlink":
            (directory / "alias").unlink()
        target.write_bytes(current)
        target.chmod(0o600)
    print("credential migration, hashing, concurrent/idempotent writes and unsafe-file rejection passed")


class LoginServer(http.server.ThreadingHTTPServer):
    mode = "valid"
    requests = []
    origin = ""
    message = ""

    def handle_error(self, request, client_address):
        if not isinstance(sys.exception(), (BrokenPipeError, ConnectionResetError, ssl.SSLError)):
            super().handle_error(request, client_address)


class LoginHandler(http.server.BaseHTTPRequestHandler):
    server: LoginServer

    def log_message(self, *_args):
        pass

    def do_POST(self):
        document = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        self.server.requests.append((self.path, document))
        identifier = document["challenge"]
        message = f"Sunshine Web UI login\nOrigin: {self.server.origin}\nChallenge: {identifier}\n"
        if self.path == "/api/ssh/challenge":
            response = {"challenge": identifier, "namespace": "sunshine-web-login", "message": message}
            if self.server.mode == "wrong-purpose":
                response["namespace"] = "different-purpose"
            if self.server.mode == "wrong-origin":
                response["message"] = message.replace(self.server.origin, "https://attacker.invalid")
            if self.server.mode == "wrong-id":
                response["challenge"] = "b" * 64
            body = json.dumps(response).encode()
            if self.server.mode == "oversized":
                body = b" " * 20001
        else:
            assert self.path == "/api/ssh/complete"
            signature = ROOT / "login.sig"
            signature.write_text(document["signature"])
            run("ssh-keygen", "-Y", "verify", "-f", ROOT / "allowed_signers", "-I", "test", "-n", "sunshine-web-login", "-s", signature, data=message)
            body = b'{"status":true}'
        self.send_response(302 if self.server.mode == "redirect" else 200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ssl.SSLError):
            pass  # Rejected/oversized responses deliberately close the client.


def ssh_login():
    key, cert, changed = [ROOT / filename for filename in ("tls.key", "tls.crt", "changed.crt")]
    run("openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", key, "-out", cert, "-subj", "/CN=fixture", "-days", "1", "-set_serial", "1")
    # Same public key, different certificate: SPKI pinning would accept this.
    run("openssl", "req", "-x509", "-key", key, "-out", changed, "-subj", "/CN=fixture", "-days", "1", "-set_serial", "2")
    identity = ROOT / "identity"
    run("ssh-keygen", "-q", "-t", "ed25519", "-N", "", "-f", identity)
    (ROOT / "allowed_signers").write_text("test " + identity.with_suffix(".pub").read_text())
    server = LoginServer(("127.0.0.1", 0), LoginHandler)
    server.origin = f"https://127.0.0.1:{server.server_port}"
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert, key)
    server.socket = context.wrap_socket(server.socket, server_side=True)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        args = [SSH, server.origin, "--challenge", "a" * 64, "-i", identity]
        run(*args, "--certificate", cert)
        assert [path for path, _ in server.requests] == ["/api/ssh/challenge", "/api/ssh/complete"]
        server.requests.clear()
        run(*args, "--certificate", changed, success=False)
        assert not server.requests, "mismatched leaf certificate must fail before sending HTTP data"
        run(*args, success=False)
        assert not server.requests, "untrusted TLS must not become insecure by default"
        for mode in ["wrong-purpose", "wrong-origin", "wrong-id", "oversized", "redirect"]:
            server.mode = mode
            server.requests.clear()
            run(*args, "--certificate", cert, success=False)
            assert len(server.requests) == 1 and server.requests[0][0] == "/api/ssh/challenge"
        for origin in ["http://127.0.0.1", "https://user@127.0.0.1", server.origin + "/path", server.origin + "?query", server.origin + "#fragment"]:
            server.requests.clear()
            run(SSH, origin, "--challenge", "a" * 64, "-i", identity, "--certificate", cert, success=False)
            assert not server.requests
    finally:
        server.shutdown()
        server.server_close()
        thread.join()
    print("real TLS leaf pin, same-key certificate rejection, destination-bound signing and response bounds passed")


def fixture(name, body, directory):
    path = directory / name
    path.write_text("#!" + sys.executable + "\n" + body)
    path.chmod(0o755)


def display():
    directory = ROOT / "display"
    directory.mkdir()
    binaries = directory / "bin"
    binaries.mkdir()
    calls = directory / "calls"
    alive = directory / "monitor"
    inventory = {"outputs": [{"name": "DP-1", "enabled": True, "currentModeId": "1", "modes": [{"id": "1", "name": "1920x1080@60"}], "pos": {"x": 0, "y": 0}, "scale": 1, "hdr": False, "wcg": False, "priority": 1}]}
    fixture("kscreen-doctor", f'''import json,sys
from pathlib import Path
with Path({str(calls)!r}).open("a") as file: file.write(json.dumps(sys.argv[1:])+"\\n")
if sys.argv[1:] == ["--json"]:
    inventory={inventory!r}
    if Path({str(alive)!r}).exists(): inventory["outputs"].append({{"name":"Virtual-Sunshine","connected":True}})
    print(json.dumps(inventory))
''', binaries)
    fixture("krfb-virtualmonitor", f'''import signal,time
from pathlib import Path
marker=Path({str(alive)!r}); marker.touch()
def stop(*_):
    marker.unlink(missing_ok=True)
    raise SystemExit(0)
signal.signal(signal.SIGTERM,stop)
while True: time.sleep(.05)
''', binaries)
    config = directory / "config.json"
    config.write_text(json.dumps({"name": "Sunshine", "primary": "DP-1", "defaultWidth": 1920, "defaultHeight": 1080, "defaultFps": 60, "scale": "1", "position": "0,0", "port": 5909, "settleSeconds": 2, "disablePrimaryOnStream": True}))
    env = {**os.environ, "PATH": str(binaries) + ":" + os.environ["PATH"], "XDG_RUNTIME_DIR": str(directory), "SUNSHINE_CLIENT_WIDTH": "2560", "SUNSHINE_CLIENT_HEIGHT": "1440", "SUNSHINE_CLIENT_FPS": "119.9", "SUNSHINE_CLIENT_HDR": "true"}
    subprocess.run([TOOLS / "virtual-display", config, "start"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True, timeout=15)
    try:
        assert alive.exists()
        commands = [json.loads(line) for line in calls.read_text().splitlines()]
        assert any("output.Virtual-Sunshine.mode.2560x1440@119" in command and "output.DP-1.disable" in command for command in commands)
        run(TOOLS / "virtual-display", config, "stop", env=env)
        wait(lambda: not alive.exists())
        commands = [json.loads(line) for line in calls.read_text().splitlines()]
        assert any("output.DP-1.mode.1920x1080@60" in command for command in commands)
        assert not (directory / "sunshine-virtual-display/state.json").exists()
    finally:
        run(TOOLS / "virtual-display", config, "stop", env=env)
    print("virtual-monitor lifecycle, client mode/HDR and physical-output restoration passed")


def slopsmith():
    directory = ROOT / "slopsmith"
    directory.mkdir()
    binaries = directory / "bin"
    binaries.mkdir()
    calls = directory / "calls"
    up = directory / "up"
    for name in ["podman", "podman-compose", "curl", "xdg-open"]:
        fixture(name, f'''import json,sys,time
from pathlib import Path
name=Path(sys.argv[0]).name
with Path({str(calls)!r}).open("a") as file: file.write(json.dumps([name,*sys.argv[1:]])+"\\n")
up=Path({str(up)!r})
if name=="podman-compose" and "up" in sys.argv:
    up.touch(); print("service log",flush=True)
    while up.exists(): time.sleep(.05)
if name=="podman-compose" and "down" in sys.argv: up.unlink(missing_ok=True)
if name=="curl" and not up.exists(): raise SystemExit(7)
''', binaries)
    dlc = directory / 'DLC: "quoted"'
    dlc.mkdir()
    config = directory / "config.json"
    config.write_text(json.dumps({"rocksmithDlc": str(dlc), "configDir": str(directory / "config"), "source": str(directory), "port": 8000}))
    env = {**os.environ, "PATH": str(binaries) + ":" + os.environ["PATH"], "XDG_RUNTIME_DIR": str(directory), "XDG_STATE_HOME": str(directory)}
    for number, expected in [(signal.SIGTERM, 143), (signal.SIGHUP, 143), (signal.SIGINT, 130)]:
        slopsmith_session(config, env, directory, calls, up, dlc, number, expected)
    print("Slopsmith single-session locking, readiness, logs and signal cleanup passed")


def slopsmith_session(config, env, directory, calls, up, dlc, number, expected):
    calls.unlink(missing_ok=True)
    session = subprocess.Popen([TOOLS / "slopsmith-runtime", config, "session"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        wait(lambda: calls.exists() and '"xdg-open"' in calls.read_text())
        assert json.loads((directory / "slopsmith/docker-compose.yml").read_text())["services"]["web"]["volumes"][0]["source"] == str(dlc)
        run(TOOLS / "slopsmith-runtime", config, "session", env=env)
        session.send_signal(number)
        assert session.wait(timeout=15) == expected
        assert not up.exists()
        assert not (directory / "slopsmith/slopsmith.pid").exists()
        commands = [json.loads(line) for line in calls.read_text().splitlines()]
        assert sum(command[0] == "podman-compose" and "up" in command for command in commands) == 1
        assert sum(command[0] == "xdg-open" for command in commands) == 2
        assert "service log" in (directory / "slopsmith/slopsmith.log").read_text()
    finally:
        if session.poll() is None:
            session.kill()
            session.wait()
        up.unlink(missing_ok=True)


def artwork():
    for name, dimensions in [("p", (600, 900)), ("landscape", (920, 430)), ("_hero", (1920, 620))]:
        document = ET.parse(ARTWORK / (name + ".svg"))
        title = document.find("{http://www.w3.org/2000/svg}text")
        assert title is not None
        assert title.text == 'A & <B> "C" \'D\''
        png = (ARTWORK / (name + ".png")).read_bytes()
        assert png[:8] == b"\x89PNG\r\n\x1a\n" and struct.unpack(">II", png[16:24]) == dimensions
    assert (ARTWORK / "_logo.png").exists()
    print("Nix SVG escaping and actual rendered artwork dimensions passed")


credentials()
ssh_login()
display()
slopsmith()
artwork()
