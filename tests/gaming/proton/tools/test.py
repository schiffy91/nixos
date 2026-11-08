"""Temporary-filesystem Proton workflows; never launch games or change host links."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time


def main():
    program = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="proton-tools-") as temporary:
        root = Path(temporary)
        config = root / "repo with spaces"
        proton = config / "config/pkgs/gaming/proton/custom"
        proton.mkdir(parents=True)
        (proton / "package.nix").write_text('let toolVersion = "test"; in {}')
        home = root / "home"
        tools = home / ".local/share/Steam/compatibilitytools.d"
        tools.mkdir(parents=True)
        store = root / "store"
        (store / "files").mkdir(parents=True)
        (store / "compatibilitytool.vdf").write_text('"display_name" "proton-custom test (Wayland SNI)"')
        active = tools / "proton-custom-test"
        active.symlink_to(store)
        asio = root / "wineasio"
        for platform, name in [("x86_64-windows", "wineasio64.dll"), ("x86_64-unix", "wineasio64.dll.so")]:
            path = asio / "lib/wine" / platform / name
            path.parent.mkdir(parents=True)
            path.write_text(name)
        assets = config / "config/pkgs/gaming/rocksmith/assets"
        assets.mkdir(parents=True)
        for name in ["wineasio32.dll", "wineasio32.dll.so"]:
            (assets / name).write_text(name)
        binaries = root / "bin"
        binaries.mkdir()
        log = root / "commands.jsonl"
        for name in ["nix", "make"]:
            script = binaries / name
            script.write_text(f"#!{sys.executable}\n" + '''import json, os, sys
with open(os.environ["COMMAND_LOG"], "a") as log: log.write(json.dumps(sys.argv) + "\\n")
if sys.argv[0].endswith("nix"): print(os.environ["ASIO"])
''')
            script.chmod(0o755)
        launcher = home / "Games/Battle.net/prefix/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe"
        launcher.parent.mkdir(parents=True)
        launcher.touch()
        umu = binaries / "umu-run"
        umu.write_text(f"#!{sys.executable}\nimport os,time\nassert not os.path.exists('/proc/self/fd/'+os.environ['TEST_FD'])\nprint(os.environ['PROTONPATH'],flush=True)\nprint(os.environ['TEST_VALUE'],flush=True)\ntime.sleep(20)\n")
        umu.chmod(0o755)
        environment = dict(os.environ, HOME=str(home), NIXOS_CONFIG_ROOT=str(config),
                           PROTON_CUSTOM_IN_DEV_SHELL="1", COMMAND_LOG=str(log), ASIO=str(asio),
                           PATH=f"{binaries}:{os.environ['PATH']}")

        def run(*arguments, fail=False, pass_fds=()):
            result = subprocess.run([program, *arguments], env=environment, capture_output=True, text=True, pass_fds=pass_fds)
            assert (result.returncode != 0) == fail, result.stdout + result.stderr
            return result

        run("dev", "status")
        assert active.resolve() == store
        run("dev", "setup")
        dev = tools / "proton-custom-test-dev"
        assert "proton-custom DEV test" in (dev / "compatibilitytool.vdf").read_text()
        assert (dev / "files/lib/wine/i386-unix/wineasio32.dll.so").is_file()
        run("dev", "use")
        assert active.resolve() == dev
        run("dev", "restore")
        assert active.resolve() == store
        run("install", "store", str(store))
        assert (proton / "src/.store-target").read_text().strip() == str(store)
        run("install", "store", str(root / "missing"), fail=True)
        assert active.resolve() == store
        run("build", "wayland", "ntdll", "wayland")
        commands = [json.loads(line) for line in log.read_text().splitlines()]
        make = [command for command in commands if command[0].endswith("make")][-1]
        assert make[1:] == ["-C", str(proton), "build", "TARGETS=dlls/winewayland.drv/all dlls/ntdll/all"]
        run("build", "invalid", fail=True)
        run("dev", "grab-frames", "--mode", "full", "--count", "1", "--output-dir", str(root / "frames"), fail=True)
        run("dev", "launch", "--env", "bad", fail=True)
        wrapper = root / "launch.wrapper.log"
        started = time.monotonic()
        with (root / "private-input").open("w") as private:
            run("dev", "launch", "--seconds", "1", "--env", "TEST_VALUE=two words=value", "--env", f"TEST_FD={private.fileno()}", "--log", str(wrapper), pass_fds=(private.fileno(),))
        assert time.monotonic() - started < 5
        assert wrapper.read_text().splitlines() == [str(dev), "two words=value"]
        umu.write_text("#!/definitely/missing/interpreter\n")
        run("dev", "launch", "--seconds", "1", "--log", str(wrapper), fail=True)
    print("PASS: Proton setup/use/restore, WineASIO, build argv, capture guard, bounded launch, descriptor isolation and failed exec")


if __name__ == "__main__":
    main()
