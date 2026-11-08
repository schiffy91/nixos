"""Exercise publishing against a fake Nix command; no cache is published."""

import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile


FAKE_NIX = r'''#!/usr/bin/env python3
import json, os, pathlib, sys
args = sys.argv[1:]
with open(os.environ["COMMAND_LOG"], "a") as log:
    log.write(json.dumps(args) + "\n")
if args[:3] == ["eval", "--json", "--file"]:
    print(json.dumps({"cache":{"name":"test-1","publicKey":"test-1:public"},
        "systems":[{"system":"x86_64-linux","attrs":["one"]}],"local":["one","two"]}))
elif args[:2] == ["eval", "--raw"]:
    print("x86_64-linux")
elif args[:2] == ["key", "generate-secret"]:
    print("test-1:secret")
elif args[:2] == ["key", "convert-secret-to-public"]:
    assert sys.stdin.read() == "test-1:secret\n"
    print("test-1:public")
elif args[0] == "build":
    if os.environ.get("FAIL_BUILD"): sys.exit(7)
    print("/nix/store/fake-output")
elif args[:2] == ["store", "sign"]:
    assert args[2:3] == ["--key-file"] and args[4:] == ["--recursive", "/nix/store/fake-output"]
elif args[0] == "copy":
    assert args[1] == "--to" and args[2].endswith("?compression=zstd")
    assert args[3:] == ["/nix/store/fake-output"]
else:
    raise AssertionError(args)
'''


def main():
    program = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="publisher-") as temporary:
        root = Path(temporary)
        (root / "flake.nix").touch()
        fake = root / "nix"
        fake.write_text(FAKE_NIX.replace("#!/usr/bin/env python3", f"#!{sys.executable}", 1))
        fake.chmod(0o755)
        log = root / "commands.jsonl"
        key = root / "private key"
        cache = root / "cache with spaces"
        environment = dict(os.environ, PATH=f"{root}:{os.environ['PATH']}",
                           NIXOS_CONFIG_ROOT=str(root), COMMAND_LOG=str(log))

        def run(*args, fail=False):
            log.write_text("")
            env = environment | ({"FAIL_BUILD": "1"} if fail else {})
            result = subprocess.run([str(program), "publish", "--key", str(key),
                                     "--dir", str(cache), *args], env=env,
                                    capture_output=True, text=True)
            assert (result.returncode != 0) == fail, result.stderr
            assert "test-1:secret" not in result.stdout + result.stderr
            return [json.loads(line) for line in log.read_text().splitlines()]

        commands = run()
        assert stat.S_IMODE(key.stat().st_mode) == 0o600
        assert key.read_text() == "test-1:secret\n"
        assert [command[-1] for command in commands if command[0] == "build"] == [f"{root}#one"]
        assert any(command[:2] == ["store", "sign"] for command in commands)
        assert any(command[0] == "copy" for command in commands)
        commands = run("--all")
        assert not any(command[:2] == ["key", "generate-secret"] for command in commands)
        assert [command[-1] for command in commands if command[0] == "build"] == [f"{root}#one", f"{root}#two"]
        commands = run("explicit")
        assert [command[-1] for command in commands if command[0] == "build"] == [f"{root}#explicit"]
        commands = run(f"--dir={cache}", f"--key={key}", "--", "explicit")
        assert [command[-1] for command in commands if command[0] == "build"] == [f"{root}#explicit"]
        commands = run(fail=True)
        assert not any(command[0] in {"store", "copy"} for command in commands)
    print("PASS: publisher targets, key creation/reuse/mode, argument boundaries and failed-build isolation")


if __name__ == "__main__":
    main()
