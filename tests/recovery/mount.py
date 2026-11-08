"""Exercise recovery decisions through real processes and isolated command fixtures."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


with tempfile.TemporaryDirectory(prefix="recovery-mount-") as temporary:
    root = Path(temporary)
    target = root / "target"
    top = root / "top"
    tools = root / "bin"
    tools.mkdir()
    for name in ("findmnt", "mount", "cryptsetup", "btrfs"):
        wrapper = tools / name
        wrapper.write_text(f"#!{sys.executable}\n" + Path(sys.argv[2]).read_text())
        wrapper.chmod(0o755)
    state_path = root / "state.json"
    root_device = root / "device"
    boot = root / "boot"
    boot.touch()
    plan = {
        "encrypted": True, "rootDevice": str(root_device), "rootPartition": "/fixture/partition", "rootName": "fixture",
        "bootPartition": str(boot), "recoveryPartition": str(root / "absent"),
        "volumes": [{"mountPoint": "/", "name": "@root", "options": "compress=zstd"}, {"mountPoint": "/home", "name": "@home", "options": ""}],
        "persist": [{"path": "/var/lib", "subvolume": "persist/var"}, {"path": "/var/lib/nested", "subvolume": "persist/nested"}, {"path": "/missing", "subvolume": "persist/absent"}],
    }
    plan_path = root / "plan.json"
    plan_path.write_text(json.dumps(plan))
    state_path.write_text(json.dumps({"root": str(root_device), "mounts": {}, "available": [str(top / "persist/var"), str(top / "persist/nested")], "commands": []}))
    environment = dict(os.environ, PATH=str(tools) + os.pathsep + os.environ["PATH"], MOUNT_FIXTURE_STATE=str(state_path))
    command = [sys.argv[1], str(plan_path), str(target), str(top)]

    def run():
        return subprocess.run(command, env=environment, capture_output=True, timeout=15)

    result = run()
    assert result.returncode == 0, result.stderr
    state = json.loads(state_path.read_text())
    mounts = [entry[-1] for entry in state["commands"] if entry[0] == "mount"]
    assert mounts == [str(target), str(target / "home"), str(top), str(target / "var/lib"), str(target / "boot")], mounts
    assert sum(entry[0] == "cryptsetup" for entry in state["commands"]) == 1
    assert (target / "tmp").stat().st_mode & 0o7777 == 0o1777
    state["commands"] = []
    state_path.write_text(json.dumps(state))
    assert run().returncode == 0
    state = json.loads(state_path.read_text())
    assert not any(entry[0] in ("mount", "cryptsetup") for entry in state["commands"])

    for changed in ({"source": str(boot)}, {"fstype": "ext4"}, {"options": "subvol=@other"}):
        initial = dict(state["mounts"][str(target)])
        state["mounts"][str(target)].update(changed)
        state["commands"] = []
        state_path.write_text(json.dumps(state))
        result = run()
        assert result.returncode and b"mounted differently" in result.stderr, result.stderr
        assert not any(entry[0] == "mount" for entry in json.loads(state_path.read_text())["commands"])
        state["mounts"][str(target)] = initial
    del state["mounts"][str(target / "boot")]
    boot.unlink()
    state_path.write_text(json.dumps(state))
    result = run()
    assert result.returncode and b"missing device" in result.stderr, result.stderr
    print("PASS: encrypted recovery, ordered mounts, idempotence, persistence selection, conflicts and required/optional devices")
