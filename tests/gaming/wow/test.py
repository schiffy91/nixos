"""Local command fixtures; do not refresh the repository's actual addon lock."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


FAKE = '''import json,os,sys
command = os.path.basename(sys.argv[0])
with open(os.environ["CALLS"],"a") as log: log.write(json.dumps([command,*sys.argv[1:]])+"\\n")
if command == "git":
    print("hash\\trefs/tags/bundle/2\\nhash\\trefs/tags/bundle/10\\nhash\\trefs/tags/bundle/10^{}")
elif command == "nix":
    if os.environ.get("FAIL_PREFETCH"): sys.exit(9)
    print(json.dumps({"hash":"sha256-fixture"}))
else:
    url=sys.argv[-1]
    if url.endswith("releases.atom"):
        print('<a href="https://github.com/Org/Addon/releases/tag/new"/><a href="https://github.com/Org/Addon/releases/tag/old"/>')
    elif url.endswith("/new"):
        print("no archive")
    else:
        tag=url.split("expanded_assets/")[1]
        print(f'<a href="/Org/Addon/releases/download/{tag}/Addon.zip"/><a href="/Org/Addon/releases/download/{tag}/Addon-forever.zip"/>')
'''


def main():
    program = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="wow-lock-") as temporary:
        root = Path(temporary)
        directory = root / "config/pkgs/gaming/wow"
        directory.mkdir(parents=True)
        (directory / "addons.json").write_text(json.dumps({
            "pinned": {"url": "https://fixture.invalid/pinned.zip"},
            "base": {"repo": "Org/Addon"},
            "forever": {"repo": "Org/Addon", "flavour": "forever"},
            "train": {"repo": "Org/Addon", "train": "bundle"},
        }))
        binaries = root / "bin"
        binaries.mkdir()
        for command in ["curl", "git", "nix"]:
            executable = binaries / command
            executable.write_text(f"#!{sys.executable}\n" + FAKE)
            executable.chmod(0o755)
        log = root / "calls.jsonl"
        environment = dict(os.environ, NIXOS_CONFIG_ROOT=str(root), CALLS=str(log), PATH=f"{binaries}:{os.environ['PATH']}")
        subprocess.run([program], env=environment, check=True)
        path = directory / "addons.lock.json"
        contents = path.read_bytes()
        owner = (path.stat().st_uid, path.stat().st_gid)
        subprocess.run([program], env=environment, check=True)
        assert (path.stat().st_uid, path.stat().st_gid) == owner
        result = json.loads(contents)
        assert result["pinned"]["url"] == "https://fixture.invalid/pinned.zip"
        assert result["base"]["url"].endswith("/old/Addon.zip")
        assert result["forever"]["url"].endswith("/old/Addon-forever.zip")
        assert result["train"]["url"].endswith("/bundle/10/Addon.zip")
        assert all(pin["hash"] == "sha256-fixture" for pin in result.values())
        failed = subprocess.run([program], env=environment | {"FAIL_PREFETCH": "1"}, capture_output=True)
        assert failed.returncode != 0 and path.read_bytes() == contents
        assert list(directory.glob(".addons.lock.*")) == []
    print("PASS: pinned/feed/train refresh, release fallback, hashes and atomic failure isolation")


if __name__ == "__main__":
    main()
