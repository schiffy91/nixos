#!/usr/bin/env python3
"""
Generated test harness for immutabilityv2.rs.

The fast path creates 1000 deterministic keep-path environments, asks the Rust
binary for its non-root plan, and verifies subvolume naming, kind resolution,
ordering, and nested-directory collapse. It intentionally does not enable the
v2 boot strategy or touch the real system.

The fake-Btrfs integration pass covers the user journeys that can be exercised
without root or real mounts: converge promotion of directories and files,
nested keep-path collapse, existing persistent subvolumes, stale NEXT cleanup
after an interrupted publish, failure before live replacement when CLEAN is
missing, runtime-disabled, clean fallback when live files are absent,
snapshot/restore modes, persistent-path collisions, deleted files, stale
file-store cleanup, symlink files, multi-volume converges, path-traversal
rejection, dry-run logging without mutation, assume-mounted dry-runs without
mount/umount, spaces, Unicode filenames, and large blobs.

Usage:
  python3 scripts/lib/test/immutabilityv2_test.py
  python3 scripts/lib/test/immutabilityv2_test.py --binary /tmp/immutabilityv2
"""
import argparse
import os
import random
import shutil
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "scripts/lib/immutabilityv2.rs"
COUNT = 1000
PERSIST_ROOT = "@persist"
SNAPSHOTS = "@snapshots"
CLEAN = "CLEAN"
VOLUMES = [
    ("@root", "/"),
    ("@home", "/home"),
    ("@var", "/var"),
]
PARTS = [
    "alex",
    ".config",
    ".local",
    "share",
    "state",
    "cache",
    "Code",
    "Steam",
    "Games",
    "Battle.net",
    "with space",
    "kdeglobals",
    "known_hosts",
    "machine-id",
    "case",
]


def run(args, **kwargs):
    return subprocess.run(args, check=True, text=True, **kwargs)


def write_executable(path: Path, text: str) -> None:
    path.write_text(text)
    path.chmod(0o755)


def compile_binary(tmpdir: Path) -> Path:
    binary = tmpdir / "immutabilityv2"
    rustc = shutil.which("rustc")
    if rustc:
        run([rustc, "--edition", "2021", "-O", "-o", str(binary), str(SOURCE)])
        return binary
    command = (
        "rustc --edition 2021 -O "
        f"-o {shlex.quote(str(binary))} {shlex.quote(str(SOURCE))}"
    )
    run(["nix-shell", "-p", "rustc", "--run", command])
    return binary


def run_rust_unit_tests(tmpdir: Path) -> None:
    binary = tmpdir / "immutabilityv2-unit"
    rustc = shutil.which("rustc")
    if rustc:
        run([rustc, "--edition", "2021", "--test", "-o", str(binary), str(SOURCE)])
    else:
        command = (
            "rustc --edition 2021 --test "
            f"-o {shlex.quote(str(binary))} {shlex.quote(str(SOURCE))}"
        )
        run(["nix-shell", "-p", "rustc", "--run", command])
    run([str(binary)])


def normalize_abs(path: str) -> tuple[str, bool]:
    trailing = len(path) > 1 and path.endswith("/")
    return (path.rstrip("/") if trailing else path), trailing


def relative_to_mount(abs_path: str, mount: str) -> str:
    if mount == "/":
        return abs_path.lstrip("/")
    prefix = mount.rstrip("/") + "/"
    assert abs_path.startswith(prefix), (abs_path, mount)
    return abs_path[len(prefix):]


def subvol_name(volume: str, mount: str, abs_path: str, rel: str) -> str:
    return f"{PERSIST_ROOT}/{abs_path.lstrip('/')}"


def file_storage_name(abs_path: str) -> str:
    return f"{PERSIST_ROOT}/.immutabilityv2/files/{abs_path.lstrip('/')}"


def is_ancestor(parent: str, child: str) -> bool:
    return child.startswith(parent) and len(child) > len(parent) and child[len(parent)] == "/"


def resolved_kind(kind: str, trailing: bool) -> str:
    if kind in ("directory", "dir"):
        return "directory"
    if kind == "auto" and trailing:
        return "directory"
    return "file"


def expected_plan(specs: list[tuple[str, str, str, str]]) -> list[tuple[str, str, str, str, str, str]]:
    normalized = []
    for volume, mount, raw_path, kind in specs:
        abs_path, trailing = normalize_abs(raw_path)
        rel = relative_to_mount(abs_path, mount)
        normalized.append({
            "volume": volume,
            "mount": mount,
            "abs": abs_path,
            "rel": rel,
            "kind": resolved_kind(kind, trailing),
        })

    normalized.sort(key=lambda item: (
        item["volume"],
        item["rel"].count("/"),
        len(item["rel"]),
        item["rel"],
    ))

    planned = []
    for item in normalized:
        covered = any(
            existing["volume"] == item["volume"]
            and existing["kind"] == "directory"
            and is_ancestor(existing["rel"], item["rel"])
            for existing in planned
        )
        if covered:
            continue
        item["subvol"] = (
            subvol_name(item["volume"], item["mount"], item["abs"], item["rel"])
            if item["kind"] == "directory"
            else file_storage_name(item["abs"])
        )
        item["payload"] = (
            item["rel"]
            if item["kind"] == "file"
            else "*"
        )
        planned.append(item)

    return [
        (item["kind"], item["volume"], item["rel"], item["abs"], item["subvol"], item["payload"])
        for item in planned
    ]


def path_for(mount: str, parts: list[str], trailing=False) -> str:
    prefix = "" if mount == "/" else mount
    path = prefix + "/" + "/".join(parts)
    return path + "/" if trailing else path


def generated_environment(index: int) -> list[tuple[str, str, str, str]]:
    rng = random.Random(0x51F15EED + index)
    volume, mount = VOLUMES[index % len(VOLUMES)]
    specs = []

    base = [
        PARTS[(index + offset) % len(PARTS)]
        for offset in range(1 + index % 4)
    ] + [f"env-{index}"]
    base_dir = path_for(mount, base, trailing=True)

    if index % 2 == 0:
        specs.append((volume, mount, base_dir, "directory" if index % 4 == 0 else "auto"))
        specs.append((volume, mount, path_for(mount, base + ["child.txt"]), "file"))
        specs.append((volume, mount, path_for(mount, base + ["nested", "state.db"]), "auto"))
    else:
        specs.append((volume, mount, path_for(mount, base + ["leaf.txt"]), "file"))

    for slot in range(1 + index % 5):
        vol, vol_mount = VOLUMES[(index + slot + 1) % len(VOLUMES)]
        depth = 1 + rng.randrange(4)
        parts = [rng.choice(PARTS), f"case-{index}", f"slot-{slot}"]
        parts.extend(rng.choice(PARTS) for _ in range(depth))
        as_dir = rng.randrange(3) == 0
        kind = rng.choice(["directory", "file", "auto"])
        if as_dir:
            kind = rng.choice(["directory", "auto"])
        specs.append((vol, vol_mount, path_for(vol_mount, parts, trailing=as_dir), kind))

    if index % 11 == 0:
        other_volume, other_mount = VOLUMES[(index + 2) % len(VOLUMES)]
        parent = path_for(other_mount, ["shared", f"env-{index}"], trailing=True)
        specs.append((other_volume, other_mount, parent, "directory"))
        specs.append((other_volume, other_mount, path_for(other_mount, ["shared", f"env-{index}", "covered"]), "file"))

    return specs


def write_spec(path: Path, specs: list[tuple[str, str, str, str]]) -> None:
    lines = [f"{volume}\t{mount}\t{abs_path}\t{kind}\n" for volume, mount, abs_path, kind in specs]
    path.write_text("".join(lines))


def run_plan(binary: Path, spec: Path) -> list[tuple[str, str, str, str, str, str]]:
    result = run([str(binary), "plan", PERSIST_ROOT, str(spec)], capture_output=True)
    if not result.stdout.strip():
        return []
    return [tuple(line.split("\t")) for line in result.stdout.strip().splitlines()]


def make_fake_tools(tmpdir: Path) -> Path:
    bindir = tmpdir / "fake-bin"
    bindir.mkdir(parents=True)

    common = r'''
import os
import shutil
import sys
from pathlib import Path

MARKER = ".fake-btrfs-subvolume"
RO = ".fake-btrfs-readonly"

def root() -> Path:
    return Path(os.environ["FAKE_BTRFS_ROOT"]).resolve()

def mark(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)
    (path / MARKER).write_text("subvolume\n")

def is_subvolume(path: Path) -> bool:
    return path.is_dir() and (path / MARKER).exists()

def remove(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)

def copy_any(src: Path, dst: Path) -> None:
    if dst.exists() or dst.is_symlink():
        remove(dst)
    dst.parent.mkdir(parents=True, exist_ok=True)
    if src.is_symlink():
        dst.symlink_to(os.readlink(src))
    elif src.is_dir():
        shutil.copytree(src, dst, symlinks=True)
    else:
        shutil.copy2(src, dst)

def copy_contents(src: Path, dst: Path) -> None:
    dst.mkdir(parents=True, exist_ok=True)
    for child in src.iterdir():
        if child.name in (MARKER, RO):
            continue
        copy_any(child, dst / child.name)
'''

    write_executable(bindir / "btrfs", "#!/usr/bin/env python3\n" + common + r'''
def rel(path: Path) -> str:
    return str(path.resolve().relative_to(root()))

def snapshot(src: Path, dst: Path) -> None:
    if dst.exists() or dst.is_symlink():
        remove(dst)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.mkdir()
    mark(dst)
    copy_contents(src, dst)

def main() -> int:
    args = sys.argv[1:]
    if os.environ.get("FAKE_COMMAND_OUTPUT") == "1":
        print("fake btrfs stdout")
        print("fake btrfs stderr", file=sys.stderr)
    if os.environ.get("FAKE_BTRFS_FAIL") and os.environ["FAKE_BTRFS_FAIL"] in " ".join(args):
        print("fake btrfs forced failure", file=sys.stderr)
        return 1
    if args[:2] == ["subvolume", "create"]:
        mark(Path(args[2]))
        return 0
    if args[:2] == ["subvolume", "delete"]:
        target = Path(args[-1])
        for path in target.rglob(MARKER):
            if path.parent != target:
                print(f"fake btrfs: child subvolume blocks delete: {path.parent}", file=sys.stderr)
                return 1
        remove(target)
        return 0
    if args[:3] == ["subvolume", "list", "-o"]:
        base = Path(args[3]).resolve()
        for path in sorted(base.rglob(MARKER)):
            subvol = path.parent
            if subvol == base:
                continue
            print(f"ID 256 gen 1 top level 5 path {rel(subvol)}")
        return 0
    if args[:2] == ["subvolume", "snapshot"]:
        snapshot(Path(args[2]), Path(args[3]))
        return 0
    if args[:2] == ["subvolume", "show"]:
        if os.environ.get("FAKE_BTRFS_SHOW_UUID") == "1":
            target = rel(Path(args[2]))
            print(f"UUID: {target}-uuid")
            print("Parent UUID: parent-uuid")
        return 0 if is_subvolume(Path(args[2])) else 1
    if args[:2] == ["property", "set"]:
        target = Path(args[-3])
        readonly = args[-1] == "true"
        marker = target / RO
        if readonly:
            marker.write_text("true\n")
        elif marker.exists():
            marker.unlink()
        return 0
    if args[:2] == ["property", "get"]:
        target = Path(args[-2])
        print("ro=true" if (target / RO).exists() else "ro=false")
        return 0
    if args[:2] == ["filesystem", "sync"]:
        return 0
    print("fake btrfs: unsupported " + " ".join(args), file=sys.stderr)
    return 2

if __name__ == "__main__":
    sys.exit(main())
''')

    write_executable(bindir / "cp", "#!/usr/bin/env python3\n" + common + r'''
def main() -> int:
    if os.environ.get("FAKE_COMMAND_OUTPUT") == "1":
        print("fake cp stdout")
        print("fake cp stderr", file=sys.stderr)
    args = [arg for arg in sys.argv[1:] if arg not in ("-a", "--reflink=always")]
    if os.environ.get("FAKE_CP_FAIL") == "1":
        print("fake cp forced failure", file=sys.stderr)
        return 1
    if len(args) != 2:
        print("fake cp: unsupported " + " ".join(sys.argv[1:]), file=sys.stderr)
        return 2
    src_raw, dst_raw = args
    if src_raw.endswith("/."):
        copy_contents(Path(src_raw[:-2]), Path(dst_raw))
    else:
        copy_any(Path(src_raw), Path(dst_raw))
    return 0

if __name__ == "__main__":
    sys.exit(main())
''')

    write_executable(bindir / "mount", r'''#!/usr/bin/env bash
set -euo pipefail
if [ "${FAKE_COMMAND_OUTPUT:-}" = 1 ]; then
	echo "fake mount stdout"
	echo "fake mount stderr" >&2
fi
if [ "${FAKE_MOUNT_FAIL:-}" = 1 ]; then
	echo "fake mount forced failure" >&2
	exit 1
fi
mkdir -p "${@: -1}"
''')
    write_executable(bindir / "umount", r'''#!/usr/bin/env bash
exit 0
''')
    return bindir


def mark_subvol(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)
    (path / ".fake-btrfs-subvolume").write_text("subvolume\n")


def write_file(path: Path, data: bytes | str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(data, bytes):
        path.write_bytes(data)
    else:
        path.write_text(data)


def read_tree(root: Path) -> dict[str, str | int | None]:
    result = {}
    for path in sorted(root.rglob("*")):
        if path.name == ".fake-btrfs-subvolume":
            continue
        rel = "/" + str(path.relative_to(root))
        if path.is_symlink():
            result[rel] = "symlink:" + os.readlink(path)
        elif path.is_file():
            data = path.read_bytes()
            result[rel] = len(data) if len(data) > 8192 else data.decode(errors="replace")
        elif path.is_dir():
            result[rel] = None
    return result


class FakeBtrfsEnv:
    def __init__(self, tmpdir: Path, binary: Path):
        self.tmpdir = tmpdir
        self.binary = binary
        self.mnt = tmpdir / "mnt"
        self.tools = make_fake_tools(tmpdir)
        self.env = os.environ.copy()
        self.env.update({
            "PATH": f"{self.tools}:{self.env['PATH']}",
            "IMMUTABILITY_MOUNT_PATH": str(self.mnt),
            "FAKE_BTRFS_ROOT": str(self.mnt),
        })
        self.mnt.mkdir()
        mark_subvol(self.mnt / SNAPSHOTS)

    def volume(self, name="@home") -> Path:
        return self.mnt / name

    def snap(self, name="@home", snap=CLEAN) -> Path:
        return self.mnt / SNAPSHOTS / name / snap

    def create_volume(self, name="@home") -> None:
        mark_subvol(self.mnt / SNAPSHOTS / name)
        for snap in (CLEAN, "A", "B", "C"):
            mark_subvol(self.snap(name, snap))
        mark_subvol(self.volume(name))

    def spec(self, name: str, rows: list[tuple[str, str, str, str]]) -> Path:
        path = self.tmpdir / f"{name}.tsv"
        write_spec(path, rows)
        return path

    def run(self, mode: str, spec: Path, pairs=("@home=/home",), check=True, dry_run=False, assume_mounted=False):
        args = [str(self.binary)]
        if dry_run:
            args.append("--dry-run")
        if assume_mounted:
            args.append("--assume-mounted")
        args.extend(["fake-device", SNAPSHOTS, CLEAN, mode, PERSIST_ROOT, str(spec), *pairs])
        return subprocess.run(
            args,
            capture_output=True,
            text=True,
            check=check,
            env=self.env,
        )


def assert_has(tree: dict, path: str, value) -> None:
    actual = tree.get(path)
    if actual != value:
        raise AssertionError(f"{path}: got {actual!r}, want {value!r}\n{tree}")


def assert_not_has(tree: dict, path: str) -> None:
    if path in tree:
        raise AssertionError(f"{path}: unexpected {tree[path]!r}\n{tree}")


def integration_reset_promotes_dirs_and_files(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-reset", binary)
    env.create_volume("@home")
    big = bytes((i % 251 for i in range(2 * 1024 * 1024)))
    write_file(env.snap() / "alex/system-default", "clean")
    write_file(env.volume() / "alex/Downloads/nested/big.bin", big)
    write_file(env.volume() / "alex/.bash_history", "user history")
    write_file(env.volume() / "alex/session.tmp", "delete me")
    spec = env.spec("reset", [
        ("@home", "/home", "/home/alex/Downloads", "auto"),
        ("@home", "/home", "/home/alex/.bash_history", "auto"),
        ("@home", "/home", "/home/alex/.bash_history", "auto"),
    ])
    env.run("reset", spec)

    live = read_tree(env.volume())
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(live, "/alex/system-default", "clean")
    assert_has(live, "/alex/.bash_history", "user history")
    assert_has(live, "/alex/Downloads", None)
    assert_not_has(live, "/alex/Downloads/nested/big.bin")
    assert_not_has(live, "/alex/session.tmp")
    assert_has(persist, "/home/alex/Downloads/nested/big.bin", len(big))


def integration_command_output_and_sync_failure(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-command-output", binary)
    env.create_volume("@home")
    env.env["FAKE_COMMAND_OUTPUT"] = "1"
    env.env["FAKE_BTRFS_FAIL"] = "filesystem sync"
    spec = env.spec("command-output", [])
    proc = env.run("reset", spec, check=False)
    output = proc.stdout + proc.stderr
    if proc.returncode == 0:
        raise AssertionError("forced filesystem sync failure should fail")
    if "fake mount stdout" not in output or "fake mount stderr" not in output:
        raise AssertionError(output)
    if "fake btrfs stdout" not in output or "fake btrfs stderr" not in output:
        raise AssertionError(output)
    if "Failed: btrfs filesystem sync" not in output:
        raise AssertionError(output)


def integration_mount_failure_fails_closed(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-mount-fail", binary)
    env.env["FAKE_MOUNT_FAIL"] = "1"
    spec = env.spec("mount-fail", [])
    proc = env.run("disabled", spec, check=False)
    output = proc.stdout + proc.stderr
    if proc.returncode == 0:
        raise AssertionError("forced mount failure should fail")
    if "Failed: mount -t btrfs" not in output:
        raise AssertionError(output)


def integration_nested_directory_collapses(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-nested", binary)
    env.create_volume("@home")
    write_file(env.volume() / "alex/.config/Code/User/settings.json", "{}")
    write_file(env.volume() / "alex/.config/Code/logs/log.txt", "log")
    spec = env.spec("nested", [
        ("@home", "/home", "/home/alex/.config/Code", "auto"),
        ("@home", "/home", "/home/alex/.config/Code/User/settings.json", "auto"),
    ])
    env.run("reset", spec)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(persist, "/home/alex/.config/Code/User/settings.json", "{}")
    assert_not_has(persist, "/.immutabilityv2/files/home/alex/.config/Code/User/settings.json")


def integration_existing_persist_survives(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-existing", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / "home/alex/Downloads")
    write_file(env.mnt / PERSIST_ROOT / "home/alex/Downloads/old.txt", "old")
    write_file(env.volume() / "alex/Downloads/new.txt", "new")
    spec = env.spec("existing", [("@home", "/home", "/home/alex/Downloads", "directory")])
    env.run("reset", spec)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(persist, "/home/alex/Downloads/old.txt", "old")
    assert_not_has(persist, "/home/alex/Downloads/new.txt")


def integration_stale_next_rebuilt(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-stale-next", binary)
    env.create_volume("@home")
    mark_subvol(env.snap("@home", "NEXT"))
    mark_subvol(env.snap("@home", "NEXT/nested-subvol"))
    write_file(env.snap("@home", "NEXT/stale.txt"), "bad")
    write_file(env.snap("@home", "NEXT/nested-subvol/child.txt"), "child")
    write_file(env.snap() / "base.txt", "clean")
    spec = env.spec("stale", [])
    env.run("reset", spec)
    live = read_tree(env.volume())
    assert_has(live, "/base.txt", "clean")
    assert_not_has(live, "/stale.txt")


def integration_failure_live_unchanged(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-fail", binary)
    env.create_volume("@home")
    shutil.rmtree(env.snap())
    write_file(env.volume() / "precious.txt", "survive")
    spec = env.spec("fail", [("@home", "/home", "/home/alex/Downloads", "directory")])
    proc = env.run("reset", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("missing CLEAN should fail")
    assert_has(read_tree(env.volume()), "/precious.txt", "survive")


def integration_second_converge_reuses_persist(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-reuse", binary)
    env.create_volume("@home")
    write_file(env.volume() / "alex/Downloads/old.txt", "old")
    spec = env.spec("reuse", [("@home", "/home", "/home/alex/Downloads", "directory")])
    env.run("reset", spec)
    write_file(env.volume() / "alex/Downloads/new-session.txt", "new")
    env.run("reset", spec)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(persist, "/home/alex/Downloads/old.txt", "old")
    assert_not_has(persist, "/home/alex/Downloads/new-session.txt")


def integration_clean_fallback_when_live_missing(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-clean-fallback", binary)
    env.create_volume("@home")
    write_file(env.snap() / "alex/Downloads/default.txt", "clean dir")
    write_file(env.snap() / "alex/.config/app.conf", "clean file")
    spec = env.spec("clean-fallback", [
        ("@home", "/home", "/home/alex/Downloads/", "auto"),
        ("@home", "/home", "/home/alex/.config/app.conf", "auto"),
    ])
    env.run("reset", spec)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    live = read_tree(env.volume())
    assert_has(persist, "/home/alex/Downloads/default.txt", "clean dir")
    assert_has(live, "/alex/.config/app.conf", "clean file")


def integration_file_restore_replaces_clean_directory(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-file-replaces-dir", binary)
    env.create_volume("@home")
    write_file(env.volume() / "alex/app.conf", "live file")
    write_file(env.snap() / "alex/app.conf/default", "clean directory")
    spec = env.spec("file-replaces-dir", [("@home", "/home", "/home/alex/app.conf", "file")])
    env.run("reset", spec)
    live = read_tree(env.volume())
    assert_has(live, "/alex/app.conf", "live file")
    assert_not_has(live, "/alex/app.conf/default")


def integration_directory_placeholder_replaces_clean_file(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-dir-replaces-file", binary)
    env.create_volume("@home")
    write_file(env.snap() / "alex/Downloads", "clean file")
    write_file(env.volume() / "alex/Downloads/live.txt", "live")
    spec = env.spec("dir-replaces-file", [("@home", "/home", "/home/alex/Downloads", "directory")])
    env.run("reset", spec)
    live = read_tree(env.volume())
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(live, "/alex/Downloads", None)
    assert_has(persist, "/home/alex/Downloads/live.txt", "live")


def integration_absent_directory_keep_creates_empty_persist(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-absent-dir", binary)
    env.create_volume("@home")
    spec = env.spec("absent-dir", [("@home", "/home", "/home/alex/Missing/", "auto")])
    env.run("reset", spec)
    assert_has(read_tree(env.volume()), "/alex/Missing", None)
    if read_tree(env.mnt / PERSIST_ROOT / "home/alex/Missing"):
        raise AssertionError(read_tree(env.mnt / PERSIST_ROOT / "home/alex/Missing"))


def integration_existing_file_store_targets_are_replaced(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-file-store-replace", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / ".immutabilityv2/files")
    write_file(env.volume() / "alex/old-file", "new file")
    write_file(env.mnt / PERSIST_ROOT / ".immutabilityv2/files/home/alex/old-file", "old file")
    write_file(env.volume() / "alex/old-dir", "new dir collision")
    write_file(env.mnt / PERSIST_ROOT / ".immutabilityv2/files/home/alex/old-dir/child", "old dir")
    spec = env.spec("file-store-replace", [
        ("@home", "/home", "/home/alex/old-file", "file"),
        ("@home", "/home", "/home/alex/old-dir", "file"),
    ])
    env.run("reset", spec)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(persist, "/.immutabilityv2/files/home/alex/old-file", "new file")
    assert_has(persist, "/.immutabilityv2/files/home/alex/old-dir", "new dir collision")
    assert_not_has(persist, "/.immutabilityv2/files/home/alex/old-dir/child")


def integration_missing_file_payload_removes_ready_targets(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-missing-file-payload", binary)
    env.create_volume("@home")
    mark_subvol(env.snap("@home", "READY"))
    write_file(env.snap("@home", "READY/.fake-btrfs-readonly"), "true\n")
    write_file(env.snap("@home", "READY/alex/missing-file"), "clean file")
    write_file(env.snap("@home", "READY/alex/missing-dir/child"), "clean dir")
    spec = env.spec("missing-file-payload", [
        ("@home", "/home", "/home/alex/missing-file", "file"),
        ("@home", "/home", "/home/alex/missing-dir", "file"),
    ])
    env.run("reset", spec)
    live = read_tree(env.volume())
    assert_not_has(live, "/alex/missing-file")
    assert_not_has(live, "/alex/missing-dir")
    assert_not_has(live, "/alex/missing-dir/child")


def integration_ready_rebuilt_when_lineage_changes(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-ready-lineage", binary)
    env.create_volume("@home")
    env.env["FAKE_BTRFS_SHOW_UUID"] = "1"
    write_file(env.volume() / "alex/Downloads/file.txt", "live")
    spec = env.spec("ready-lineage", [("@home", "/home", "/home/alex/Downloads", "directory")])
    env.run("reset", spec)
    proc = env.run("reset", spec)
    if "btrfs subvolume show" not in proc.stdout:
        raise AssertionError(proc.stdout + proc.stderr)


def integration_ready_checked_when_clean_show_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-ready-clean-show-fails", binary)
    env.create_volume("@home")
    (env.snap("@home", CLEAN) / ".fake-btrfs-subvolume").unlink()
    mark_subvol(env.snap("@home", "READY"))
    write_file(env.snap("@home", "READY/.fake-btrfs-readonly"), "true\n")
    spec = env.spec("ready-clean-show-fails", [])
    env.run("reset", spec)


def integration_ready_publish_rename_failure_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-ready-rename-fail", binary)
    env.create_volume("@home")
    parent = env.mnt / SNAPSHOTS / "@home"
    spec = env.spec("ready-rename-fail", [])
    os.chmod(parent, 0o555)
    try:
        proc = env.run("reset", spec, check=False)
    finally:
        os.chmod(parent, 0o755)
    if proc.returncode == 0:
        raise AssertionError("READY publish rename failure should fail")


def integration_snapshot_and_restore_modes(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-modes", binary)
    env.create_volume("@home")
    write_file(env.volume() / "live.txt", "live")
    write_file(env.snap("@home", "A") / "previous.txt", "previous")
    write_file(env.snap("@home", "B") / "penultimate.txt", "penultimate")
    write_file(env.snap("@home", "C") / "discarded.txt", "discarded")
    spec = env.spec("modes", [])
    env.run("snapshot-only", spec)
    assert_has(read_tree(env.snap("@home", "A")), "/live.txt", "live")
    env.run("restore-penultimate", spec)
    assert_has(read_tree(env.volume()), "/penultimate.txt", "penultimate")
    env.run("restore-b", spec)
    assert_has(read_tree(env.volume()), "/previous.txt", "previous")
    env.run("restore-a", spec)
    assert_has(read_tree(env.volume()), "/live.txt", "live")


def integration_rotate_failures_are_fatal(binary: Path, tmpdir: Path) -> None:
    cases = [
        ("case-rotate-b-fail", ["C"], "B"),
        ("case-rotate-a-fail", ["B", "C"], "A"),
        ("case-rotate-capture-fail", ["A", "B", "C"], "CAPTURE"),
    ]
    for name, removed, label in cases:
        env = FakeBtrfsEnv(tmpdir / name, binary)
        env.create_volume("@home")
        write_file(env.volume() / "live.txt", "live")
        for snap in removed:
            shutil.rmtree(env.snap("@home", snap))
        parent = env.mnt / SNAPSHOTS / "@home"
        spec = env.spec(name, [])
        os.chmod(parent, 0o555)
        try:
            proc = env.run("snapshot-only", spec, check=False)
        finally:
            os.chmod(parent, 0o755)
        if proc.returncode == 0:
            raise AssertionError(f"rotate {label} failure should fail")


def integration_restore_missing_generation_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-restore-missing", binary)
    env.create_volume("@home")
    shutil.rmtree(env.snap("@home", "C"))
    write_file(env.volume() / "live.txt", "live")
    spec = env.spec("restore-missing", [])
    proc = env.run("restore-c", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("restore-c should fail when C is missing")
    assert_has(read_tree(env.volume()), "/live.txt", "live")


def integration_snapshot_only_live_missing_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-snapshot-live-missing", binary)
    env.create_volume("@home")
    shutil.rmtree(env.volume())
    spec = env.spec("snapshot-live-missing", [])
    proc = env.run("snapshot-only", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("snapshot-only should fail when live is missing")


def integration_bad_persist_collision_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-collision", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    write_file(env.mnt / PERSIST_ROOT / "home/alex/Downloads/not-a-subvol", "collision")
    write_file(env.volume() / "alex/Downloads/file.txt", "value")
    spec = env.spec("collision", [("@home", "/home", "/home/alex/Downloads", "directory")])
    proc = env.run("reset", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("directory persist collision should fail")


def integration_persist_publish_rename_failure_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-persist-rename-fail", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    parent = env.mnt / PERSIST_ROOT / "home/alex"
    parent.mkdir(parents=True)
    write_file(env.volume() / "alex/Downloads/file", "value")
    spec = env.spec("persist-rename-fail", [("@home", "/home", "/home/alex/Downloads", "directory")])
    os.chmod(parent, 0o555)
    try:
        proc = env.run("reset", spec, check=False)
    finally:
        os.chmod(parent, 0o755)
    if proc.returncode == 0:
        raise AssertionError("persist publish rename failure should fail")


def integration_special_paths_and_deleted_files(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-special", binary)
    env.create_volume("@home")
    write_file(env.snap() / "alex/with space/café.txt", "clean unicode")
    write_file(env.volume() / "alex/with space/café.txt", "user unicode")
    spec = env.spec("special", [
        ("@home", "/home", "/home/alex/with space/café.txt", "file"),
        ("@home", "/home", "/home/alex/deleted.txt", "file"),
    ])
    env.run("reset", spec)
    live = read_tree(env.volume())
    assert_has(live, "/alex/with space/café.txt", "user unicode")
    assert_not_has(live, "/alex/deleted.txt")


def integration_deleted_file_removes_old_file_store(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-delete-store", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / ".immutabilityv2/files")
    write_file(env.mnt / PERSIST_ROOT / ".immutabilityv2/files/home/alex/gone.txt", "old")
    spec = env.spec("deleted-store", [("@home", "/home", "/home/alex/gone.txt", "file")])
    env.run("reset", spec)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    live = read_tree(env.volume())
    assert_not_has(persist, "/.immutabilityv2/files/home/alex/gone.txt")
    assert_not_has(live, "/alex/gone.txt")


def integration_removed_file_spec_deletes_stale_payload(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-removed-file-spec", binary)
    env.create_volume("@home")
    write_file(env.volume() / "alex/keep.txt", "keep")
    spec = env.spec("with-file", [("@home", "/home", "/home/alex/keep.txt", "file")])
    env.run("reset", spec)
    assert_has(read_tree(env.mnt / PERSIST_ROOT), "/.immutabilityv2/files/home/alex/keep.txt", "keep")
    empty = env.spec("without-file", [])
    env.run("reset", empty)
    assert_not_has(read_tree(env.mnt / PERSIST_ROOT), "/.immutabilityv2/files/home/alex/keep.txt")


def integration_obsolete_directory_persist_is_deleted(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-obsolete-dir", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / "home/alex/obsolete")
    write_file(env.mnt / PERSIST_ROOT / "home/alex/obsolete/old.txt", "old")
    spec = env.spec("obsolete-dir", [])
    env.run("reset", spec)
    if (env.mnt / PERSIST_ROOT / "home/alex/obsolete").exists():
        raise AssertionError("obsolete persist subvolume should leave @persist")


def integration_meta_child_persist_is_kept(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-meta-child", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / ".immutabilityv2/extra")
    spec = env.spec("meta-child", [])
    env.run("reset", spec)
    if not (env.mnt / PERSIST_ROOT / ".immutabilityv2/extra").exists():
        raise AssertionError("managed metadata child should stay under @persist")


def integration_obsolete_persist_rename_failure_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-obsolete-rename-fail", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / "home/alex/obsolete")
    mark_subvol(env.mnt / "@trash")
    spec = env.spec("obsolete-rename-fail", [])
    os.chmod(env.mnt / "@trash", 0o555)
    try:
        proc = env.run("reset", spec, check=False)
    finally:
        os.chmod(env.mnt / "@trash", 0o755)
    if proc.returncode == 0:
        raise AssertionError("obsolete persist rename failure should fail")


def integration_stale_payload_parent_cleanup_stops_on_sibling(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-stale-parent-sibling", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / ".immutabilityv2/files")
    write_file(env.volume() / "alex/sibling", "sibling")
    write_file(env.mnt / PERSIST_ROOT / ".immutabilityv2/files/home/alex/gone", "gone")
    write_file(env.mnt / PERSIST_ROOT / ".immutabilityv2/files/home/alex/sibling", "sibling")
    blocked = env.mnt / PERSIST_ROOT / ".immutabilityv2/files/blocked"
    blocked.mkdir()
    os.chmod(blocked, 0o000)
    spec = env.spec("stale-parent-sibling", [("@home", "/home", "/home/alex/sibling", "file")])
    try:
        env.run("reset", spec)
    finally:
        os.chmod(blocked, 0o755)
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_not_has(persist, "/.immutabilityv2/files/home/alex/gone")
    assert_has(persist, "/.immutabilityv2/files/home/alex/sibling", "sibling")


def integration_trash_delete_budget_is_bounded(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-trash-budget", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / "@trash")
    mark_subvol(env.mnt / "@trash/old-a")
    mark_subvol(env.mnt / "@trash/old-b")
    env.env["IMMUTABILITY_TRASH_DELETE_BUDGET"] = "1"
    spec = env.spec("trash-budget", [])
    env.run("reset", spec)
    remaining = [path for path in (env.mnt / "@trash").iterdir() if path.is_dir()]
    if len(remaining) != 2:
        raise AssertionError(remaining)


def integration_symlink_file_is_preserved(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-symlink", binary)
    env.create_volume("@home")
    target = env.volume() / "alex/link"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.symlink_to("target-file")
    spec = env.spec("symlink", [("@home", "/home", "/home/alex/link", "file")])
    env.run("reset", spec)
    assert_has(read_tree(env.volume()), "/alex/link", "symlink:target-file")


def integration_multi_volume_parallel(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-multi-volume", binary)
    env.create_volume("@root")
    env.create_volume("@home")
    write_file(env.volume("@root") / "etc/nixos/configuration.nix", "root live")
    write_file(env.volume("@home") / "alex/Downloads/file.txt", "home live")
    spec = env.spec("multi", [
        ("@root", "/", "/etc/nixos", "directory"),
        ("@home", "/home", "/home/alex/Downloads", "directory"),
    ])
    env.run("reset", spec, pairs=("@root=/", "@home=/home"))
    persist = read_tree(env.mnt / PERSIST_ROOT)
    assert_has(persist, "/etc/nixos/configuration.nix", "root live")
    assert_has(persist, "/home/alex/Downloads/file.txt", "home live")
    assert_has(read_tree(env.volume("@root")), "/etc/nixos", None)
    assert_has(read_tree(env.volume("@home")), "/alex/Downloads", None)


def integration_disabled_runtime_touches_no_state(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-disabled", binary)
    spec = env.spec("disabled", [("@home", "/home", "/home/alex/Downloads", "directory")])
    env.run("disabled", spec)
    if (env.mnt / PERSIST_ROOT).exists():
        raise AssertionError("disabled mode should not create the persist root")


def integration_volume_args_can_be_inferred_from_spec(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-infer-volumes", binary)
    env.create_volume("@home")
    write_file(env.volume() / "alex/file", "value")
    spec = env.spec("infer-volumes", [
        ("@home", "/home", "/home/alex/file", "file"),
        ("@home", "/home", "/home/alex/file", "file"),
    ])
    if len(run_plan(binary, spec)) != 1:
        raise AssertionError("duplicate file keep path should plan once")
    proc = subprocess.run(
        [str(binary), "fake-device", SNAPSHOTS, CLEAN, "disabled", PERSIST_ROOT, str(spec)],
        capture_output=True,
        text=True,
        check=True,
        env=env.env,
    )
    if "subvolumes=@home" not in proc.stdout:
        raise AssertionError(proc.stdout + proc.stderr)


def integration_live_missing_uses_pending_next(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-live-missing-next", binary)
    env.create_volume("@home")
    shutil.rmtree(env.volume())
    mark_subvol(env.snap("@home", "NEXT"))
    write_file(env.snap("@home", "NEXT/pending.txt"), "pending")
    spec = env.spec("live-missing-next", [])
    env.run("reset", spec)
    if read_tree(env.volume()):
        raise AssertionError(read_tree(env.volume()))
    assert_has(read_tree(env.snap("@home", "A")), "/pending.txt", "pending")


def integration_live_missing_restores_from_a(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-live-missing-a", binary)
    env.create_volume("@home")
    shutil.rmtree(env.volume())
    write_file(env.snap("@home", "A/restored.txt"), "from A")
    spec = env.spec("live-missing-a", [])
    env.run("reset", spec)
    if read_tree(env.volume()):
        raise AssertionError(read_tree(env.volume()))
    assert_has(read_tree(env.snap("@home", "A")), "/restored.txt", "from A")


def integration_live_missing_without_recovery_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-live-missing-no-recovery", binary)
    env.create_volume("@home")
    shutil.rmtree(env.volume())
    shutil.rmtree(env.snap("@home", "A"))
    spec = env.spec("live-missing-no-recovery", [])
    proc = env.run("reset", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("missing live without NEXT or A should fail")


def integration_live_missing_next_publish_failure_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-live-missing-next-publish-fail", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / ".immutabilityv2/files")
    mark_subvol(env.mnt / "@staging")
    mark_subvol(env.mnt / "@trash")
    shutil.rmtree(env.volume())
    mark_subvol(env.snap("@home", "NEXT"))
    spec = env.spec("live-missing-next-publish-fail", [])
    os.chmod(env.mnt, 0o555)
    try:
        proc = env.run("reset", spec, check=False)
    finally:
        os.chmod(env.mnt, 0o755)
    if proc.returncode == 0:
        raise AssertionError("missing live NEXT publish failure should fail")


def integration_busy_live_publish_leaves_next(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-busy-live", binary)
    env.create_volume("@home")
    mark_subvol(env.mnt / PERSIST_ROOT)
    mark_subvol(env.mnt / PERSIST_ROOT / ".immutabilityv2/files")
    mark_subvol(env.mnt / "@staging")
    mark_subvol(env.mnt / "@trash")
    os.chmod(env.mnt / "@trash", 0o555)
    try:
        spec = env.spec("busy-live", [])
        env.run("reset", spec)
        if not env.snap("@home", "NEXT").exists():
            raise AssertionError("busy publish should leave NEXT for a later converge")
    finally:
        os.chmod(env.mnt / "@trash", 0o755)


def integration_dry_run_logs_without_mutation(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-dry-run", binary)
    env.create_volume("@home")
    write_file(env.snap() / "alex/default.txt", "clean")
    write_file(env.volume() / "alex/Downloads/file.txt", "live")
    write_file(env.volume() / "alex/.bash_history", "history")
    write_file(env.volume() / "remove-me.txt", "still here")
    spec = env.spec("dry-run", [
        ("@home", "/home", "/home/alex/Downloads", "directory"),
        ("@home", "/home", "/home/alex/.bash_history", "file"),
    ])
    proc = env.run("reset", spec, dry_run=True)
    output = proc.stdout + proc.stderr
    if "dry_run=true" not in output:
        raise AssertionError(output)
    if "DRY btrfs subvolume create" not in output:
        raise AssertionError(output)
    if "DRY btrfs subvolume snapshot" not in output:
        raise AssertionError(output)
    if "DRY cp --reflink=always -a" not in output or "NEXT/alex/.bash_history" not in output:
        raise AssertionError(output)
    assert_has(read_tree(env.volume()), "/remove-me.txt", "still here")
    assert_not_has(read_tree(env.volume()), "/alex/default.txt")
    if (env.mnt / PERSIST_ROOT).exists():
        raise AssertionError("dry-run should not create the persist root")


def integration_assume_mounted_dry_run_skips_mounts(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-assume-mounted", binary)
    env.create_volume("@home")
    write_file(env.snap() / "alex/default.txt", "clean")
    write_file(env.volume() / "alex/Downloads/file.txt", "live")
    spec = env.spec("assume-mounted", [("@home", "/home", "/home/alex/Downloads", "directory")])
    proc = env.run("reset", spec, dry_run=True, assume_mounted=True)
    output = proc.stdout + proc.stderr
    if "assume_mounted=true" not in output:
        raise AssertionError(output)
    if "mount -t btrfs" in output or "umount -R" in output:
        raise AssertionError(output)
    if "DRY btrfs subvolume create" not in output:
        raise AssertionError(output)
    if (env.mnt / PERSIST_ROOT).exists():
        raise AssertionError("assume-mounted dry-run should not create the persist root")


def integration_assume_mounted_requires_dry_run(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-assume-mounted-real", binary)
    env.create_volume("@home")
    spec = env.spec("assume-mounted-real", [])
    proc = env.run("reset", spec, assume_mounted=True, check=False)
    if proc.returncode == 0:
        raise AssertionError("--assume-mounted without --dry-run should fail")
    if "only supported with --dry-run" not in (proc.stdout + proc.stderr):
        raise AssertionError(proc.stdout + proc.stderr)


def integration_assume_mounted_requires_existing_mount_path(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-assume-mounted-missing", binary)
    spec = env.spec("assume-mounted-missing", [])
    env.env["IMMUTABILITY_MOUNT_PATH"] = str(tmpdir / "missing-mount")
    proc = env.run("reset", spec, dry_run=True, assume_mounted=True, check=False)
    if proc.returncode == 0:
        raise AssertionError("--assume-mounted should require the mount path")
    if "mount path must already exist" not in (proc.stdout + proc.stderr):
        raise AssertionError(proc.stdout + proc.stderr)


def integration_invalid_paths_are_rejected(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-invalid", binary)
    bad_keep = env.spec("bad-keep", [("@home", "/home", "/home/alex/../escape", "file")])
    bad_root_keep = env.spec("bad-root-keep", [("@home", "/home", "/", "directory")])
    bad_mount = env.spec("bad-mount", [("@home", "home", "/home/alex/file", "file")])
    bad_empty_mount = env.spec("bad-empty-mount", [("@home", "////", "/home/alex/file", "file")])
    bad_volume = env.spec("bad-volume", [("../bad", "/home", "/home/alex/file", "file")])
    for spec in (bad_keep, bad_root_keep, bad_mount, bad_empty_mount, bad_volume):
        proc = subprocess.run(
            [str(binary), "plan", PERSIST_ROOT, str(spec)],
            capture_output=True,
            text=True,
            check=False,
        )
        if proc.returncode == 0:
            raise AssertionError(f"invalid spec unexpectedly passed: {spec.read_text()}")

    good = env.spec("bad-persist-root", [("@home", "/home", "/home/alex/file", "file")])
    proc = subprocess.run(
        [str(binary), "fake-device", SNAPSHOTS, CLEAN, "../outside", str(good), "@home=/home"],
        capture_output=True,
        text=True,
        check=False,
        env=env.env,
    )
    if proc.returncode == 0:
        raise AssertionError("invalid persist root unexpectedly passed")
    if (tmpdir / "outside").exists():
        raise AssertionError("invalid persist root created an outside path")

    bad_kind = env.spec("bad-kind", [("@home", "/home", "/home/alex/file", "weird")])
    proc = subprocess.run(
        [str(binary), "plan", PERSIST_ROOT, str(bad_kind)],
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode == 0:
        raise AssertionError("invalid keep kind unexpectedly passed")
    bad_fields = tmpdir / "bad-fields.tsv"
    bad_fields.write_text("@home\t/home\t/home/alex/file\n")
    proc = subprocess.run(
        [str(binary), "plan", PERSIST_ROOT, str(bad_fields)],
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode == 0:
        raise AssertionError("invalid field count unexpectedly passed")
    proc = subprocess.run([str(binary), "plan"], capture_output=True, text=True, check=False)
    if proc.returncode == 0:
        raise AssertionError("plan usage error unexpectedly passed")
    proc = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
    if proc.returncode == 0:
        raise AssertionError("runtime usage error unexpectedly passed")
    proc = subprocess.run(
        [str(binary), "fake-device", SNAPSHOTS, "bad/name", "reset", PERSIST_ROOT, str(good), "@home=/home"],
        capture_output=True,
        text=True,
        check=False,
        env=env.env,
    )
    if proc.returncode == 0:
        raise AssertionError("invalid clean name unexpectedly passed")
    proc = subprocess.run(
        [str(binary), "fake-device", SNAPSHOTS, CLEAN, "reset", PERSIST_ROOT, str(good), "@home"],
        capture_output=True,
        text=True,
        check=False,
        env=env.env,
    )
    if proc.returncode == 0:
        raise AssertionError("invalid volume argument unexpectedly passed")
    proc = subprocess.run(
        [str(binary), "fake-device", SNAPSHOTS, CLEAN, "not-a-mode", PERSIST_ROOT, str(good), "@home=/home"],
        capture_output=True,
        text=True,
        check=False,
        env=env.env,
    )
    if proc.returncode == 0:
        raise AssertionError("unknown mode unexpectedly passed")


def integration_persist_root_must_be_subvolume(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-persist-root-kind", binary)
    env.create_volume("@home")
    (env.mnt / PERSIST_ROOT).mkdir()
    write_file(env.volume() / "alex/file", "value")
    spec = env.spec("persist-root-kind", [("@home", "/home", "/home/alex/file", "file")])
    proc = env.run("reset", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("regular-directory persist root should fail")


def integration_persist_root_file_collision_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-persist-root-file", binary)
    env.create_volume("@home")
    write_file(env.mnt / PERSIST_ROOT, "not a directory")
    spec = env.spec("persist-root-file", [("@home", "/home", "/home/alex/file", "file")])
    proc = env.run("reset", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("file persist root collision should fail")


def integration_snapshot_only_requires_clean(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-snapshot-missing-clean", binary)
    env.create_volume("@home")
    shutil.rmtree(env.snap())
    spec = env.spec("snapshot-missing-clean", [])
    proc = env.run("snapshot-only", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("snapshot-only should fail when CLEAN is missing")


def integration_plan_invalid_persist_root_fails(binary: Path, tmpdir: Path) -> None:
    spec = tmpdir / "invalid-plan.tsv"
    spec.write_text("")
    proc = subprocess.run(
        [str(binary), "plan", "/", str(spec)],
        capture_output=True,
        text=True,
    )
    if proc.returncode == 0:
        raise AssertionError("plan should reject an empty persist root")


def integration_mount_path_file_fails(binary: Path, tmpdir: Path) -> None:
    env = FakeBtrfsEnv(tmpdir / "case-mount-path-file", binary)
    mount_file = env.tmpdir / "not-a-directory"
    write_file(mount_file, "file")
    env.env["IMMUTABILITY_MOUNT_PATH"] = str(mount_file)
    spec = env.spec("mount-path-file", [])
    proc = env.run("disabled", spec, check=False)
    if proc.returncode == 0:
        raise AssertionError("mount path file should fail before mounting")


def run_integrations(binary: Path, tmpdir: Path) -> None:
    cases = [
        integration_reset_promotes_dirs_and_files,
        integration_command_output_and_sync_failure,
        integration_mount_failure_fails_closed,
        integration_nested_directory_collapses,
        integration_existing_persist_survives,
        integration_stale_next_rebuilt,
        integration_failure_live_unchanged,
        integration_second_converge_reuses_persist,
        integration_clean_fallback_when_live_missing,
        integration_file_restore_replaces_clean_directory,
        integration_directory_placeholder_replaces_clean_file,
        integration_absent_directory_keep_creates_empty_persist,
        integration_existing_file_store_targets_are_replaced,
        integration_missing_file_payload_removes_ready_targets,
        integration_ready_rebuilt_when_lineage_changes,
        integration_ready_checked_when_clean_show_fails,
        integration_ready_publish_rename_failure_fails,
        integration_snapshot_and_restore_modes,
        integration_rotate_failures_are_fatal,
        integration_restore_missing_generation_fails,
        integration_snapshot_only_live_missing_fails,
        integration_bad_persist_collision_fails,
        integration_persist_publish_rename_failure_fails,
        integration_special_paths_and_deleted_files,
        integration_deleted_file_removes_old_file_store,
        integration_removed_file_spec_deletes_stale_payload,
        integration_obsolete_directory_persist_is_deleted,
        integration_meta_child_persist_is_kept,
        integration_obsolete_persist_rename_failure_fails,
        integration_stale_payload_parent_cleanup_stops_on_sibling,
        integration_trash_delete_budget_is_bounded,
        integration_symlink_file_is_preserved,
        integration_multi_volume_parallel,
        integration_disabled_runtime_touches_no_state,
        integration_volume_args_can_be_inferred_from_spec,
        integration_live_missing_uses_pending_next,
        integration_live_missing_restores_from_a,
        integration_live_missing_without_recovery_fails,
        integration_live_missing_next_publish_failure_fails,
        integration_busy_live_publish_leaves_next,
        integration_dry_run_logs_without_mutation,
        integration_assume_mounted_dry_run_skips_mounts,
        integration_assume_mounted_requires_dry_run,
        integration_assume_mounted_requires_existing_mount_path,
        integration_invalid_paths_are_rejected,
        integration_persist_root_must_be_subvolume,
        integration_persist_root_file_collision_fails,
        integration_snapshot_only_requires_clean,
        integration_plan_invalid_persist_root_fails,
        integration_mount_path_file_fails,
    ]
    for case in cases:
        case(binary, tmpdir)
    print(f"{len(cases)} fake-btrfs integration journeys passed")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="immutabilityv2-test-") as tmp:
        tmpdir = Path(tmp)
        run_rust_unit_tests(tmpdir)
        binary = args.binary or compile_binary(tmpdir)
        failed = []

        for index in range(COUNT):
            specs = generated_environment(index)
            spec_file = tmpdir / f"environment-{index:04}.tsv"
            write_spec(spec_file, specs)
            expected = expected_plan(specs)
            actual = run_plan(binary, spec_file)
            if actual != expected:
                failed.append(index)
                print(f"FAIL environment-{index:04}")
                print("spec:")
                print(spec_file.read_text(), end="")
                print("expected:")
                for row in expected:
                    print("\t".join(row))
                print("actual:")
                for row in actual:
                    print("\t".join(row))
                break

        if failed:
            sys.exit(1)
        print(f"{COUNT} immutabilityv2 plan environments passed")
        run_integrations(binary, tmpdir)


if __name__ == "__main__":
    main()
