"""Exercise timeout dispatch through a nested graph, including failure cleanup."""
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
import time

root = Path.cwd()


def running(pid, started):
    try:
        fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
        return fields[19] == started and fields[0] not in {"Z", "X"}
    except FileNotFoundError:
        return False


for mode in ("ordinary", "stubborn"):
    with tempfile.TemporaryDirectory(prefix="nixos-host-timeout-") as temporary:
        directory = Path(temporary)
        marker = directory / "pids.json"
        command = shlex.join(["python3", str(root / "tests/harness/timeout/blocked.py"), str(marker), mode])
        (directory / "probe.json").write_text(json.dumps({
            "name": "timeout-probe", "state": "timeout-probe", "stateMaterial": temporary,
            "workDir": str(directory / "state"),
            "operations": [{"op": "host", "command": command, "timeout": 1}]}))
        graph = directory / "graph.json"
        graph.write_text(json.dumps({"name": "timeout-probe", "default": ["probe"],
                                    "nodes": [{"id": "probe", "spec": "probe.json"}]}))
        process = subprocess.Popen([str(root / "build/nixosctl"), "graph", str(graph), "run", "probe"],
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                                   start_new_session=True)
        started = time.monotonic()
        try:
            output, _ = process.communicate(timeout=10)
            assert marker.exists(), "The host command never started: " + output
            assert process.returncode != 0 and "host command failed" in output, output
            assert .8 <= time.monotonic() - started < 8, "Host timeout was not honored"
            records = json.loads(marker.read_text())
            for _ in range(40):
                if not any(running(int(pid), stamp) for pid, stamp in records.items()):
                    break
                time.sleep(.05)
            assert not any(running(int(pid), stamp) for pid, stamp in records.items()), "Timed-out children survived"
        finally:
            # A broken timeout implementation must not leak the regression's own
            # processes. Restrict cleanup to the recorded PID/start identities.
            if marker.exists():
                for pid, stamp in json.loads(marker.read_text()).items():
                    if running(int(pid), stamp):
                        os.kill(int(pid), signal.SIGKILL)
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
            process.communicate(timeout=5)

print("PASS: host deadlines stop commands and their children")
