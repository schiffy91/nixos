#!/usr/bin/env python3
"""
Test harness for immutability.rs.

Creates a real BTRFS loop device, sets up the full subvolume structure,
populates it with deterministic test data, runs a binary, and verifies
the resulting volume has the correct content.

Usage:
  sudo python3 test.py old    # test original binary
  sudo python3 test.py new    # test new binary
  sudo python3 test.py both   # test both and compare
"""
import os, sys, shutil, subprocess, tempfile, textwrap
from pathlib import Path

LOOP_SIZE  = "512M"
SNAPSHOTS  = "@snapshots"
CLEAN_NAME = "CLEAN"
VOL        = "@home"
VOL2       = "@root"

# ---------------------------------------------------------------------------
# Shell helpers
# ---------------------------------------------------------------------------

class Shell:
    @classmethod
    def run(cls, *args, check=True, capture=False):
        result = subprocess.run(list(args), capture_output=capture, text=True)
        if check and result.returncode != 0:
            stderr = result.stderr.strip() if capture else ""
            raise RuntimeError(f"Command failed: {' '.join(args)}\n{stderr}")
        return result

    @classmethod
    def out(cls, *args):
        return cls.run(*args, capture=True).stdout.strip()

# ---------------------------------------------------------------------------
# BTRFS environment
# ---------------------------------------------------------------------------

class BtrfsEnv:
    def __init__(self, workdir: Path):
        self.workdir  = workdir
        self.img      = workdir / "disk.img"
        self.mnt      = workdir / "mnt"
        self.loop_dev = None

    def setup(self):
        self.mnt.mkdir(parents=True, exist_ok=True)
        Shell.run("truncate", "-s", LOOP_SIZE, str(self.img))
        Shell.run("mkfs.btrfs", "-q", str(self.img))
        self.loop_dev = Shell.out("losetup", "--find", "--show", str(self.img))
        Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                  self.loop_dev, str(self.mnt))
        self._create_volume_tree(VOL)
        Shell.run("umount", str(self.mnt))

    def add_volume(self, vol_name: str):
        Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                  self.loop_dev, str(self.mnt))
        self._create_volume_tree(vol_name)
        Shell.run("umount", str(self.mnt))

    def teardown(self):
        try:
            Shell.run("umount", "-R", str(self.mnt), check=False)
            if self.loop_dev:
                Shell.run("losetup", "-d", self.loop_dev, check=False)
        except Exception:
            pass

    def _sv(self, *parts):
        return str(self.mnt.joinpath(*parts))

    def _create_volume_tree(self, vol_name: str):
        snap_root = self.mnt / SNAPSHOTS
        if not snap_root.exists():
            Shell.run("btrfs", "subvolume", "create", str(snap_root))
        Shell.run("btrfs", "subvolume", "create", self._sv(SNAPSHOTS, vol_name))
        for snap in (CLEAN_NAME, "PREVIOUS", "PENULTIMATE"):
            Shell.run("btrfs", "subvolume", "create", self._sv(SNAPSHOTS, vol_name, snap))
        Shell.run("btrfs", "subvolume", "create", self._sv(vol_name))

    def _read_dir(self, path: Path, skip_sentinel: bool = True) -> dict:
        result = {}
        for f in sorted(path.rglob("*")):
            if skip_sentinel and f.name == ".boot-ready":
                continue
            rel = "/" + str(f.relative_to(path))
            if f.is_symlink():
                try:    result[rel] = f.read_text()
                except: result[rel] = "<unreadable-symlink>"
            elif f.is_file():
                try:    result[rel] = f.read_text()
                except: result[rel] = "<binary>"
            elif f.is_dir():
                result[rel] = None
        return result

    def read_live(self, vol_name: str = VOL) -> dict:
        Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                  self.loop_dev, str(self.mnt))
        result = self._read_dir(Path(self._sv(vol_name)))
        Shell.run("umount", str(self.mnt))
        return result

    def read_snapshot(self, vol_name: str, snap_name: str) -> dict:
        Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                  self.loop_dev, str(self.mnt))
        result = self._read_dir(Path(self._sv(SNAPSHOTS, vol_name, snap_name)))
        Shell.run("umount", str(self.mnt))
        return result

    def sentinel_exists(self, vol_name: str = VOL) -> bool:
        Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                  self.loop_dev, str(self.mnt))
        exists = (Path(self._sv(vol_name)) / ".boot-ready").exists()
        Shell.run("umount", str(self.mnt))
        return exists

    def write_to_live(self, vol_name: str, files: dict):
        Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                  self.loop_dev, str(self.mnt))
        root = Path(self._sv(vol_name))
        for rel_path, content in files.items():
            dest = root / rel_path.lstrip("/")
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_text(content)
        Shell.run("umount", str(self.mnt))

# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------

class TestCase:
    def __init__(self, name, spec, filter_lines, expected, mode="reset",
                 post_setup=None,
                 check_snapshots=None,
                 check_sentinel=False,
                 sentinel_absent=False,
                 expected2=None,
                 boot2_live_extra=None,
                 check_snapshots2=None):
        self.name             = name
        self.spec             = spec
        self.filter_lines     = filter_lines
        self.expected         = expected
        self.mode             = mode
        self.post_setup       = post_setup      # fn(targets: dict[str,Path]) while mounted
        self.check_snapshots  = check_snapshots  # {snap_name: {path:content}} after first run
        self.check_sentinel   = check_sentinel   # assert .boot-ready IS in @home
        self.sentinel_absent  = sentinel_absent  # assert .boot-ready is NOT in @home
        self.expected2        = expected2        # run binary again and check this
        self.boot2_live_extra = boot2_live_extra # files to add to @home before second run
        self.check_snapshots2 = check_snapshots2 # {snap_name: {path:content}} after second run

def make_filter(lines: list) -> str:
    return "\n".join(lines) + "\n"

# Helper used in post_setup lambdas to delete a btrfs subvolume while mounted.
def _del_subvol(path: Path):
    subprocess.run(["btrfs", "subvolume", "delete", str(path)], capture_output=True)

TESTS = [

    # ======================================================================
    # BASIC CORRECTNESS
    # ======================================================================

    TestCase(
        name="non_persistent_session_file_disappears",
        spec={"CLEAN": {}, "PREVIOUS": {"/s": "x"}, "PENULTIMATE": {}, "LIVE": {"/s": "x"}},
        filter_lines=["- *"],
        expected={},
    ),
    TestCase(
        name="clean_only_file_preserved",
        spec={"CLEAN": {"/c": "cv"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["- *"],
        expected={"/c": "cv"},
    ),
    TestCase(
        name="non_persistent_file_reset_to_clean_version",
        spec={"CLEAN": {"/f": "clean"}, "PREVIOUS": {"/f": "session"}, "PENULTIMATE": {}, "LIVE": {"/f": "session"}},
        filter_lines=["- *"],
        expected={"/f": "clean"},
    ),
    TestCase(
        name="persistent_file_preserved_from_live",
        spec={"CLEAN": {"/p": "clean"}, "PREVIOUS": {"/p": "user"}, "PENULTIMATE": {}, "LIVE": {"/p": "user"}},
        filter_lines=["+ /p", "- *"],
        expected={"/p": "user"},
    ),
    TestCase(
        name="persistent_file_only_in_live_not_clean",
        spec={"CLEAN": {}, "PREVIOUS": {"/new": "uc"}, "PENULTIMATE": {}, "LIVE": {"/new": "uc"}},
        filter_lines=["+ /new", "- *"],
        expected={"/new": "uc"},
    ),
    TestCase(
        name="persistent_directory_with_contents_preserved",
        spec={
            "CLEAN":       {"/.cache/e": "cc"},
            "PREVIOUS":    {"/.cache/e": "rc", "/.cache/x": "xd"},
            "PENULTIMATE": {},
            "LIVE":        {"/.cache/e": "rc", "/.cache/x": "xd"},
        },
        filter_lines=["+ /.cache", "+ /.cache/", "+ /.cache/**", "- *"],
        expected={"/.cache": None, "/.cache/e": "rc", "/.cache/x": "xd"},
    ),
    TestCase(
        name="persistent_path_leaves_non_persistent_sibling_reset",
        spec={
            "CLEAN":       {"/.config/app/s": "ca", "/.config/other/s": "co"},
            "PREVIOUS":    {"/.config/app/s": "ua", "/.config/other/s": "uo"},
            "PENULTIMATE": {},
            "LIVE":        {"/.config/app/s": "ua", "/.config/other/s": "uo"},
        },
        filter_lines=["+ /.config/", "+ /.config/app", "+ /.config/app/", "+ /.config/app/**", "- *"],
        expected={
            "/.config": None, "/.config/app": None, "/.config/app/s": "ua",
            "/.config/other": None, "/.config/other/s": "co",
        },
    ),
    TestCase(
        name="persistent_path_parent_missing_from_clean",
        spec={"CLEAN": {}, "PREVIOUS": {"/.cert/vpn": "key"}, "PENULTIMATE": {}, "LIVE": {"/.cert/vpn": "key"}},
        filter_lines=["+ /.cert/", "+ /.cert/vpn", "- *"],
        expected={"/.cert": None, "/.cert/vpn": "key"},
    ),

    # ======================================================================
    # DELETION SCENARIOS
    # ======================================================================

    TestCase(
        name="persistent_file_deleted_in_session_gets_clean_fallback",
        spec={"CLEAN": {"/imp": "cv"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["+ /imp", "- *"],
        expected={"/imp": "cv"},
    ),
    TestCase(
        name="persistent_file_deleted_not_in_clean_stays_absent",
        spec={"CLEAN": {}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["+ /ghost", "- *"],
        expected={},
    ),
    TestCase(
        name="clean_file_absent_from_live_still_restored",
        spec={"CLEAN": {"/sys": "sv"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["- *"],
        expected={"/sys": "sv"},
    ),
    TestCase(
        name="session_file_not_in_clean_disappears",
        spec={"CLEAN": {}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {"/tmp_file": "junk"}},
        filter_lines=["- *"],
        expected={},
    ),
    TestCase(
        name="all_files_modified_non_persistent_all_reset",
        spec={
            "CLEAN":       {f"/f{i}": f"c{i}" for i in range(10)},
            "PREVIOUS":    {f"/f{i}": f"s{i}" for i in range(10)},
            "PENULTIMATE": {},
            "LIVE":        {f"/f{i}": f"s{i}" for i in range(10)},
        },
        filter_lines=["- *"],
        expected={f"/f{i}": f"c{i}" for i in range(10)},
    ),
    TestCase(
        name="clean_has_many_files_none_persistent_all_restored",
        spec={
            "CLEAN":       {f"/c{i}": f"cv{i}" for i in range(20)},
            "PREVIOUS":    {},
            "PENULTIMATE": {},
            "LIVE":        {},
        },
        filter_lines=["- *"],
        expected={f"/c{i}": f"cv{i}" for i in range(20)},
    ),

    # ======================================================================
    # FILTER EDGE CASES
    # ======================================================================

    TestCase(
        name="empty_filter_produces_clean_state",
        spec={
            "CLEAN":       {"/a": "ca", "/b": "cb"},
            "PREVIOUS":    {"/a": "sa", "/c": "sc"},
            "PENULTIMATE": {},
            "LIVE":        {"/a": "sa", "/c": "sc"},
        },
        filter_lines=["- *"],
        expected={"/a": "ca", "/b": "cb"},
    ),
    TestCase(
        name="filter_dir_and_glob_lines_only_yields_no_persistent_paths",
        # Only `+ /path/` and `+ /path/**` lines — parse_persistent_paths strips these.
        # Result: pure CLEAN state even though filter references paths.
        spec={"CLEAN": {"/base": "bv"}, "PREVIOUS": {"/base": "sv"}, "PENULTIMATE": {}, "LIVE": {"/base": "sv"}},
        filter_lines=["+ /base/", "+ /base/**", "- *"],
        expected={"/base": "bv"},
    ),
    TestCase(
        name="filter_with_non_plus_lines_ignored",
        # Lines not starting with `+ /` are ignored. `# comment`, blank, `- /x` all filtered.
        spec={"CLEAN": {"/p": "cv", "/q": "qv"}, "PREVIOUS": {"/p": "pv"}, "PENULTIMATE": {}, "LIVE": {"/p": "pv"}},
        filter_lines=["# this is a comment", "", "- /p", "+ /p", "- *"],
        expected={"/p": "pv", "/q": "qv"},
    ),
    TestCase(
        name="filter_duplicate_path_idempotent",
        spec={"CLEAN": {"/p": "cv"}, "PREVIOUS": {"/p": "uv"}, "PENULTIMATE": {}, "LIVE": {"/p": "uv"}},
        filter_lines=["+ /p", "+ /p", "- *"],
        expected={"/p": "uv"},
    ),
    TestCase(
        name="filter_thirty_persistent_paths_all_preserved",
        spec={
            "CLEAN":       {f"/p{i}": f"cv{i}" for i in range(30)},
            "PREVIOUS":    {f"/p{i}": f"uv{i}" for i in range(30)},
            "PENULTIMATE": {},
            "LIVE":        {f"/p{i}": f"uv{i}" for i in range(30)},
        },
        filter_lines=[f"+ /p{i}" for i in range(30)] + ["- *"],
        expected={f"/p{i}": f"uv{i}" for i in range(30)},
    ),
    TestCase(
        name="filter_fifty_persistent_paths_all_preserved",
        spec={
            "CLEAN":       {f"/x{i}": f"cv{i}" for i in range(50)},
            "PREVIOUS":    {f"/x{i}": f"uv{i}" for i in range(50)},
            "PENULTIMATE": {},
            "LIVE":        {f"/x{i}": f"uv{i}" for i in range(50)},
        },
        filter_lines=[f"+ /x{i}" for i in range(50)] + ["- *"],
        expected={f"/x{i}": f"uv{i}" for i in range(50)},
    ),

    # ======================================================================
    # NON-PERSISTENT DIRECTORY SCENARIOS
    # ======================================================================

    TestCase(
        name="non_persistent_dir_only_in_live_disappears",
        spec={"CLEAN": {}, "PREVIOUS": {"/Projects/f": "x"}, "PENULTIMATE": {}, "LIVE": {"/Projects/f": "x"}},
        filter_lines=["- *"],
        expected={},
    ),
    TestCase(
        name="non_persistent_dir_content_fully_reset_to_clean",
        spec={
            "CLEAN":       {"/tmp_dir/a": "ca"},
            "PREVIOUS":    {"/tmp_dir/a": "sa", "/tmp_dir/b": "sb"},
            "PENULTIMATE": {},
            "LIVE":        {"/tmp_dir/a": "sa", "/tmp_dir/b": "sb"},
        },
        filter_lines=["- *"],
        expected={"/tmp_dir": None, "/tmp_dir/a": "ca"},
    ),
    TestCase(
        name="non_persistent_nested_dirs_all_gone",
        spec={
            "CLEAN":       {},
            "PREVIOUS":    {"/a/b/c/f": "v"},
            "PENULTIMATE": {},
            "LIVE":        {"/a/b/c/f": "v"},
        },
        filter_lines=["- *"],
        expected={},
    ),

    # ======================================================================
    # DEEP NESTING
    # ======================================================================

    TestCase(
        name="deep_nested_persistent_path_missing_intermediates",
        spec={"CLEAN": {}, "PREVIOUS": {"/a/b/c/d/secret": "dv"}, "PENULTIMATE": {}, "LIVE": {"/a/b/c/d/secret": "dv"}},
        filter_lines=["+ /a/b/c/d/secret", "- *"],
        expected={"/a": None, "/a/b": None, "/a/b/c": None, "/a/b/c/d": None, "/a/b/c/d/secret": "dv"},
    ),
    TestCase(
        name="persistent_path_depth_8",
        spec={"CLEAN": {}, "PREVIOUS": {"/a/b/c/d/e/f/g/h/file": "deep"}, "PENULTIMATE": {}, "LIVE": {"/a/b/c/d/e/f/g/h/file": "deep"}},
        filter_lines=["+ /a/b/c/d/e/f/g/h/file", "- *"],
        expected={
            "/a": None, "/a/b": None, "/a/b/c": None, "/a/b/c/d": None,
            "/a/b/c/d/e": None, "/a/b/c/d/e/f": None, "/a/b/c/d/e/f/g": None,
            "/a/b/c/d/e/f/g/h": None, "/a/b/c/d/e/f/g/h/file": "deep",
        },
    ),
    TestCase(
        name="non_persistent_sibling_at_each_depth_level_gets_reset",
        spec={
            "CLEAN": {
                "/a/sibling": "cs1",
                "/a/b/sibling": "cs2",
                "/a/b/c/sibling": "cs3",
                "/a/b/c/d/persist": "cv",
            },
            "PREVIOUS": {
                "/a/sibling": "ss1",
                "/a/b/sibling": "ss2",
                "/a/b/c/sibling": "ss3",
                "/a/b/c/d/persist": "uv",
            },
            "PENULTIMATE": {},
            "LIVE": {
                "/a/sibling": "ss1",
                "/a/b/sibling": "ss2",
                "/a/b/c/sibling": "ss3",
                "/a/b/c/d/persist": "uv",
            },
        },
        filter_lines=["+ /a/b/c/d/persist", "- *"],
        expected={
            "/a": None, "/a/sibling": "cs1",
            "/a/b": None, "/a/b/sibling": "cs2",
            "/a/b/c": None, "/a/b/c/sibling": "cs3",
            "/a/b/c/d": None, "/a/b/c/d/persist": "uv",
        },
    ),
    TestCase(
        name="persistent_dir_deep_with_mixed_content",
        spec={
            "CLEAN":       {"/data/cache": "cc"},
            "PREVIOUS":    {"/data/cache": "rc", "/data/log": "rl", "/data/sub/f": "rsf"},
            "PENULTIMATE": {},
            "LIVE":        {"/data/cache": "rc", "/data/log": "rl", "/data/sub/f": "rsf"},
        },
        filter_lines=["+ /data", "+ /data/", "+ /data/**", "- *"],
        expected={"/data": None, "/data/cache": "rc", "/data/log": "rl", "/data/sub": None, "/data/sub/f": "rsf"},
    ),

    # ======================================================================
    # MULTIPLE INDEPENDENT PERSISTENT PATHS
    # ======================================================================

    TestCase(
        name="multiple_persistent_paths_independent_parents",
        spec={
            "CLEAN":       {"/.ssh/known_hosts": "ch", "/.ssh/eph": "ce", "/Downloads/f": "cd"},
            "PREVIOUS":    {"/.ssh/known_hosts": "rh", "/.ssh/eph": "se", "/Downloads/f": "rd", "/Downloads/x": "sx"},
            "PENULTIMATE": {},
            "LIVE":        {"/.ssh/known_hosts": "rh", "/.ssh/eph": "se", "/Downloads/f": "rd", "/Downloads/x": "sx"},
        },
        filter_lines=["+ /.ssh/", "+ /.ssh/known_hosts", "+ /Downloads", "+ /Downloads/", "+ /Downloads/**", "- *"],
        expected={
            "/.ssh": None, "/.ssh/known_hosts": "rh", "/.ssh/eph": "ce",
            "/Downloads": None, "/Downloads/f": "rd", "/Downloads/x": "sx",
        },
    ),
    TestCase(
        name="ten_persistent_files_all_preserved",
        spec={
            "CLEAN":       {f"/f{i}": f"cv{i}" for i in range(10)},
            "PREVIOUS":    {f"/f{i}": f"uv{i}" for i in range(10)},
            "PENULTIMATE": {},
            "LIVE":        {f"/f{i}": f"uv{i}" for i in range(10)},
        },
        filter_lines=[f"+ /f{i}" for i in range(10)] + ["- *"],
        expected={f"/f{i}": f"uv{i}" for i in range(10)},
    ),
    TestCase(
        name="persistent_paths_sharing_parent_some_not_persistent",
        spec={
            "CLEAN":       {"/home/alice": "ca", "/home/bob": "cb", "/home/shared": "cs"},
            "PREVIOUS":    {"/home/alice": "ua", "/home/bob": "ub", "/home/shared": "us"},
            "PENULTIMATE": {},
            "LIVE":        {"/home/alice": "ua", "/home/bob": "ub", "/home/shared": "us"},
        },
        filter_lines=["+ /home/alice", "- *"],
        expected={"/home": None, "/home/alice": "ua", "/home/bob": "cb", "/home/shared": "cs"},
    ),
    TestCase(
        name="persistent_and_non_persistent_mixed_under_same_dir",
        spec={
            "CLEAN":       {"/dir/persist": "cp", "/dir/ephemeral": "ce"},
            "PREVIOUS":    {"/dir/persist": "up", "/dir/ephemeral": "ue"},
            "PENULTIMATE": {},
            "LIVE":        {"/dir/persist": "up", "/dir/ephemeral": "ue"},
        },
        filter_lines=["+ /dir/persist", "- *"],
        expected={"/dir": None, "/dir/persist": "up", "/dir/ephemeral": "ce"},
    ),
    TestCase(
        name="persistent_path_is_entire_top_level_dir",
        spec={
            "CLEAN":       {"/data/base": "cb"},
            "PREVIOUS":    {"/data/base": "ub", "/data/new": "un", "/data/sub/f": "usf"},
            "PENULTIMATE": {},
            "LIVE":        {"/data/base": "ub", "/data/new": "un", "/data/sub/f": "usf"},
        },
        filter_lines=["+ /data", "+ /data/", "+ /data/**", "- *"],
        expected={"/data": None, "/data/base": "ub", "/data/new": "un", "/data/sub": None, "/data/sub/f": "usf"},
    ),

    # ======================================================================
    # TYPE CHANGES
    # ======================================================================

    TestCase(
        name="persistent_path_type_changed_file_to_dir",
        spec={"CLEAN": {"/t": "file"}, "PREVIOUS": {"/t/inside": "dir"}, "PENULTIMATE": {}, "LIVE": {"/t/inside": "dir"}},
        filter_lines=["+ /t", "+ /t/", "+ /t/**", "- *"],
        expected={"/t": None, "/t/inside": "dir"},
    ),
    TestCase(
        name="persistent_path_type_changed_dir_to_file",
        spec={"CLEAN": {"/t/inside": "dir"}, "PREVIOUS": {"/t": "file"}, "PENULTIMATE": {}, "LIVE": {"/t": "file"}},
        filter_lines=["+ /t", "- *"],
        expected={"/t": "file"},
    ),
    TestCase(
        name="non_persistent_symlink_in_live_disappears",
        spec={"CLEAN": {"/real": "rv"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {"/real": "rv"}},
        filter_lines=["- *"],
        post_setup=lambda t: (t["LIVE"] / "link").symlink_to("real"),
        expected={"/real": "rv"},
    ),
    TestCase(
        name="clean_has_symlink_non_persistent_symlink_restored",
        spec={"CLEAN": {"/target": "tv", "/link": "cv"}, "PREVIOUS": {"/target": "tv"}, "PENULTIMATE": {}, "LIVE": {"/target": "tv", "/link": "lv"}},
        filter_lines=["- *"],
        # CLEAN has /link. LIVE replaced it. Not persistent, so CLEAN version wins.
        expected={"/target": "tv", "/link": "cv"},
    ),
    TestCase(
        name="persistent_file_replaced_by_symlink_in_live_preserved",
        spec={"CLEAN": {"/target": "tv"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {"/target": "tv"}},
        filter_lines=["+ /link", "- *"],
        post_setup=lambda t: (t["LIVE"] / "link").symlink_to("target"),
        expected={"/target": "tv", "/link": "tv"},
    ),

    # ======================================================================
    # SYMLINK SCENARIOS
    # ======================================================================

    TestCase(
        name="persistent_symlink_to_valid_target_preserved",
        spec={"CLEAN": {"/target": "rv"}, "PREVIOUS": {"/target": "rv"}, "PENULTIMATE": {}, "LIVE": {"/target": "rv"}},
        filter_lines=["+ /link", "- *"],
        post_setup=lambda t: (t["LIVE"] / "link").symlink_to("target"),
        expected={"/target": "rv", "/link": "rv"},
    ),
    TestCase(
        name="persistent_dangling_symlink_preserved",
        spec={"CLEAN": {}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["+ /dangling", "- *"],
        post_setup=lambda t: (t["LIVE"] / "dangling").symlink_to("/nonexistent/path"),
        expected={"/dangling": "<unreadable-symlink>"},
    ),
    TestCase(
        name="persistent_symlink_to_relative_path",
        spec={"CLEAN": {"/target": "rv"}, "PREVIOUS": {"/target": "rv"}, "PENULTIMATE": {}, "LIVE": {"/target": "rv"}},
        filter_lines=["+ /rellink", "- *"],
        post_setup=lambda t: (t["LIVE"] / "rellink").symlink_to("./target"),
        expected={"/target": "rv", "/rellink": "rv"},
    ),
    TestCase(
        name="persistent_symlink_chain_link_to_link_to_file",
        spec={"CLEAN": {"/file": "fv"}, "PREVIOUS": {"/file": "fv"}, "PENULTIMATE": {}, "LIVE": {"/file": "fv"}},
        filter_lines=["+ /link1", "+ /link2", "- *"],
        post_setup=lambda t: (
            t["LIVE"] / "link2").__class__(str(t["LIVE"] / "link2")).symlink_to("file") or
            (t["LIVE"] / "link1").symlink_to("link2"),
        expected={"/file": "fv", "/link1": "fv", "/link2": "fv"},
    ),
    TestCase(
        name="non_persistent_dangling_symlink_disappears",
        spec={"CLEAN": {}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["- *"],
        post_setup=lambda t: (t["LIVE"] / "dangling").symlink_to("/nonexistent"),
        expected={},
    ),

    # ======================================================================
    # SPECIAL FILENAMES
    # ======================================================================

    TestCase(
        name="persistent_hidden_dotfile",
        spec={"CLEAN": {}, "PREVIOUS": {"/.gitconfig": "[user]\nemail=a@b.com"}, "PENULTIMATE": {}, "LIVE": {"/.gitconfig": "[user]\nemail=a@b.com"}},
        filter_lines=["+ /.gitconfig", "- *"],
        expected={"/.gitconfig": "[user]\nemail=a@b.com"},
    ),
    TestCase(
        name="persistent_hidden_dotdir_with_content",
        spec={"CLEAN": {}, "PREVIOUS": {"/.local/share/app/data": "dv"}, "PENULTIMATE": {}, "LIVE": {"/.local/share/app/data": "dv"}},
        filter_lines=["+ /.local", "+ /.local/", "+ /.local/**", "- *"],
        expected={"/.local": None, "/.local/share": None, "/.local/share/app": None, "/.local/share/app/data": "dv"},
    ),
    TestCase(
        name="special_characters_spaces_parens_in_filename",
        spec={
            "CLEAN":       {"/My Documents/report (v2).txt": "cr"},
            "PREVIOUS":    {"/My Documents/report (v2).txt": "ur"},
            "PENULTIMATE": {},
            "LIVE":        {"/My Documents/report (v2).txt": "ur"},
        },
        filter_lines=["+ /My Documents", "+ /My Documents/", "+ /My Documents/**", "- *"],
        expected={"/My Documents": None, "/My Documents/report (v2).txt": "ur"},
    ),
    TestCase(
        name="filename_with_dollar_sign",
        spec={"CLEAN": {}, "PREVIOUS": {"/file$var": "dv"}, "PENULTIMATE": {}, "LIVE": {"/file$var": "dv"}},
        filter_lines=["+ /file$var", "- *"],
        expected={"/file$var": "dv"},
    ),
    TestCase(
        name="filename_with_unicode_characters",
        spec={"CLEAN": {}, "PREVIOUS": {"/café.txt": "bon"}, "PENULTIMATE": {}, "LIVE": {"/café.txt": "bon"}},
        filter_lines=["+ /café.txt", "- *"],
        expected={"/café.txt": "bon"},
    ),
    TestCase(
        name="filename_with_hyphen_at_start",
        spec={"CLEAN": {}, "PREVIOUS": {"/-hyphen-start": "hv"}, "PENULTIMATE": {}, "LIVE": {"/-hyphen-start": "hv"}},
        filter_lines=["+ /-hyphen-start", "- *"],
        expected={"/-hyphen-start": "hv"},
    ),
    TestCase(
        name="filename_with_many_non_persistent_all_reset",
        spec={
            "CLEAN":       {f"/c{i}": f"cv{i}" for i in range(50)},
            "PREVIOUS":    {f"/s{i}": f"sv{i}" for i in range(50)},
            "PENULTIMATE": {},
            "LIVE":        {f"/s{i}": f"sv{i}" for i in range(50)},
        },
        filter_lines=["- *"],
        expected={f"/c{i}": f"cv{i}" for i in range(50)},
    ),

    # ======================================================================
    # EMPTY AND MINIMAL
    # ======================================================================

    TestCase(
        name="completely_empty_volume_stays_empty",
        spec={"CLEAN": {}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["- *"],
        expected={},
    ),
    TestCase(
        name="persistent_empty_file_zero_bytes",
        spec={"CLEAN": {}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["+ /empty", "- *"],
        post_setup=lambda t: (t["LIVE"] / "empty").write_bytes(b""),
        expected={"/empty": ""},
    ),
    TestCase(
        name="persistent_empty_directory",
        spec={"CLEAN": {}, "PREVIOUS": {"/emptydir": None}, "PENULTIMATE": {}, "LIVE": {"/emptydir": None}},
        filter_lines=["+ /emptydir", "+ /emptydir/", "+ /emptydir/**", "- *"],
        expected={"/emptydir": None},
    ),
    TestCase(
        name="single_persistent_path_volume_otherwise_clean",
        spec={"CLEAN": {"/a": "av", "/b": "bv"}, "PREVIOUS": {"/a": "ua", "/b": "ub"}, "PENULTIMATE": {}, "LIVE": {"/a": "ua", "/b": "ub"}},
        filter_lines=["+ /a", "- *"],
        expected={"/a": "ua", "/b": "bv"},
    ),

    # ======================================================================
    # SNAPSHOT STATE VERIFICATION (check_snapshots after first run)
    # ======================================================================

    TestCase(
        name="after_reset_previous_snapshot_contains_live_state",
        spec={
            "CLEAN":       {"/p": "cv"},
            "PREVIOUS":    {"/p": "uv", "/s": "sv"},
            "PENULTIMATE": {},
            "LIVE":        {"/p": "uv", "/s": "sv"},
        },
        filter_lines=["+ /p", "- *"],
        expected={"/p": "uv"},
        check_snapshots={"PREVIOUS": {"/p": "uv", "/s": "sv"}},
    ),
    TestCase(
        name="after_reset_penultimate_contains_old_previous_state",
        spec={
            "CLEAN":       {"/p": "cv"},
            "PREVIOUS":    {"/p": "old_uv"},
            "PENULTIMATE": {"/p": "even_older"},
            "LIVE":        {"/p": "new_uv"},
        },
        filter_lines=["+ /p", "- *"],
        expected={"/p": "new_uv"},
        # Before reset, PREVIOUS had "old_uv". After reset, PENULTIMATE = snapshot of old PREVIOUS.
        check_snapshots={"PREVIOUS": {"/p": "new_uv"}, "PENULTIMATE": {"/p": "old_uv"}},
    ),
    TestCase(
        name="after_snapshot_only_previous_contains_live_state",
        mode="snapshot-only",
        spec={
            "CLEAN":       {"/base": "bv"},
            "PREVIOUS":    {"/base": "bv", "/s": "sv"},
            "PENULTIMATE": {},
            "LIVE":        {"/base": "bv", "/s": "sv"},
        },
        filter_lines=["- *"],
        expected={"/base": "bv", "/s": "sv"},
        check_snapshots={"PREVIOUS": {"/base": "bv", "/s": "sv"}},
    ),
    TestCase(
        name="after_snapshot_only_penultimate_rotated_from_previous",
        mode="snapshot-only",
        spec={
            "CLEAN":       {},
            "PREVIOUS":    {"/old_prev": "opv"},
            "PENULTIMATE": {"/old_penu": "opu"},
            "LIVE":        {"/live": "lv"},
        },
        filter_lines=["- *"],
        expected={"/live": "lv"},
        check_snapshots={
            "PREVIOUS":    {"/live": "lv"},
            "PENULTIMATE": {"/old_prev": "opv"},
        },
    ),

    # ======================================================================
    # BOOTSTRAP: MISSING PREVIOUS / PENULTIMATE
    # ======================================================================

    TestCase(
        name="bootstrap_both_snapshots_missing_bootstraps_from_clean",
        spec={
            "CLEAN":       {"/sys": "sv"},
            "PREVIOUS":    {"/sys": "sv"},
            "PENULTIMATE": {"/sys": "sv"},
            "LIVE":        {"/sys": "sv", "/persist": "pv"},
        },
        filter_lines=["+ /persist", "- *"],
        # post_setup deletes PREVIOUS and PENULTIMATE to simulate first-ever boot.
        post_setup=lambda t: (_del_subvol(t["PREVIOUS"]), _del_subvol(t["PENULTIMATE"])),
        expected={"/sys": "sv", "/persist": "pv"},
        check_snapshots={
            "PREVIOUS":    {"/sys": "sv", "/persist": "pv"},
            "PENULTIMATE": {"/sys": "sv"},
        },
    ),
    TestCase(
        name="bootstrap_only_previous_missing",
        spec={
            "CLEAN":       {"/sys": "sv"},
            "PREVIOUS":    {"/sys": "sv"},
            "PENULTIMATE": {"/old": "ov"},
            "LIVE":        {"/sys": "sv", "/p": "pv"},
        },
        filter_lines=["+ /p", "- *"],
        post_setup=lambda t: _del_subvol(t["PREVIOUS"]),
        expected={"/sys": "sv", "/p": "pv"},
        check_snapshots={"PREVIOUS": {"/sys": "sv", "/p": "pv"}},
    ),
    TestCase(
        name="bootstrap_only_penultimate_missing",
        spec={
            "CLEAN":       {"/sys": "sv"},
            "PREVIOUS":    {"/p": "pv"},
            "PENULTIMATE": {"/p": "pv"},
            "LIVE":        {"/p": "pv"},
        },
        filter_lines=["+ /p", "- *"],
        post_setup=lambda t: _del_subvol(t["PENULTIMATE"]),
        expected={"/p": "pv", "/sys": "sv"},
        check_snapshots={"PENULTIMATE": {"/p": "pv"}},
    ),

    # ======================================================================
    # SENTINEL CHECKS
    # ======================================================================

    TestCase(
        name="sentinel_written_after_reset",
        spec={"CLEAN": {"/f": "v"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["- *"],
        expected={"/f": "v"},
        check_sentinel=True,
    ),
    TestCase(
        name="sentinel_absent_after_snapshot_only",
        mode="snapshot-only",
        spec={"CLEAN": {"/f": "v"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {"/f": "v"}},
        filter_lines=["- *"],
        expected={"/f": "v"},
        sentinel_absent=True,
    ),
    TestCase(
        name="sentinel_absent_after_disabled",
        mode="disabled",
        spec={"CLEAN": {"/f": "v"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {"/data": "dv"}},
        filter_lines=["- *"],
        expected={"/data": "dv"},
        sentinel_absent=True,
    ),
    TestCase(
        name="sentinel_absent_after_restore_previous",
        mode="restore-previous",
        spec={"CLEAN": {}, "PREVIOUS": {"/restored": "rv"}, "PENULTIMATE": {}, "LIVE": {"/current": "cv"}},
        filter_lines=["- *"],
        expected={"/restored": "rv"},
        sentinel_absent=True,
    ),

    # ======================================================================
    # INCOMPLETE BOOT / STALE CURRENT
    # ======================================================================

    TestCase(
        name="stale_current_no_sentinel_discarded_and_rebuilt",
        spec={"CLEAN": {"/f": "cv"}, "PREVIOUS": {"/f": "uv"}, "PENULTIMATE": {}, "LIVE": {"/f": "uv"}},
        filter_lines=["+ /f", "- *"],
        post_setup=lambda t: (t["CURRENT"] / "stale_junk").write_text("wrong"),
        expected={"/f": "uv"},
    ),
    TestCase(
        name="stale_current_with_wrong_persistent_content_discarded",
        spec={"CLEAN": {"/p": "cv"}, "PREVIOUS": {"/p": "correct"}, "PENULTIMATE": {}, "LIVE": {"/p": "correct"}},
        filter_lines=["+ /p", "- *"],
        # CURRENT has wrong value for the persistent path — must be discarded.
        post_setup=lambda t: (t["CURRENT"] / "p").write_text("WRONG_STALE_VALUE"),
        expected={"/p": "correct"},
    ),
    TestCase(
        name="current_with_good_sentinel_still_rebuilt_correctly",
        spec={"CLEAN": {"/f": "cv"}, "PREVIOUS": {"/f": "uv"}, "PENULTIMATE": {}, "LIVE": {"/f": "uv"}},
        filter_lines=["+ /f", "- *"],
        # CURRENT has .boot-ready (good prior boot) — binary still rebuilds it on next reset.
        post_setup=lambda t: (
            (t["CURRENT"] / "f").write_text("old_current_value"),
            (t["CURRENT"] / ".boot-ready").write_text(""),
        ),
        expected={"/f": "uv"},
    ),

    # ======================================================================
    # MODES
    # ======================================================================

    TestCase(
        name="snapshot_only_live_unchanged",
        mode="snapshot-only",
        spec={"CLEAN": {"/base": "bv"}, "PREVIOUS": {"/base": "bv", "/s": "sv"}, "PENULTIMATE": {}, "LIVE": {"/base": "bv", "/s": "sv"}},
        filter_lines=["- *"],
        expected={"/base": "bv", "/s": "sv"},
    ),
    TestCase(
        name="disabled_mode_live_unchanged",
        mode="disabled",
        spec={"CLEAN": {"/sys": "sv"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {"/data": "dv", "/user": "uv"}},
        filter_lines=["- *"],
        expected={"/data": "dv", "/user": "uv"},
    ),
    TestCase(
        name="restore_previous_home_becomes_previous",
        mode="restore-previous",
        spec={
            "CLEAN":       {"/clean_only": "cv"},
            "PREVIOUS":    {"/restored": "rv", "/extra": "ev"},
            "PENULTIMATE": {"/penu": "pv"},
            "LIVE":        {"/current": "cur"},
        },
        filter_lines=["- *"],
        expected={"/restored": "rv", "/extra": "ev"},
    ),
    TestCase(
        name="restore_penultimate_home_becomes_penultimate",
        mode="restore-penultimate",
        spec={
            "CLEAN":       {"/clean_only": "cv"},
            "PREVIOUS":    {"/prev": "pv"},
            "PENULTIMATE": {"/penu_a": "pa", "/penu_b": "pb"},
            "LIVE":        {"/current": "cur"},
        },
        filter_lines=["- *"],
        expected={"/penu_a": "pa", "/penu_b": "pb"},
    ),
    TestCase(
        name="restore_previous_clears_all_live_content",
        mode="restore-previous",
        spec={
            "CLEAN":       {},
            "PREVIOUS":    {"/only_this": "prev_content"},
            "PENULTIMATE": {},
            "LIVE":        {"/a": "av", "/b": "bv", "/c/d": "cdv"},
        },
        filter_lines=["- *"],
        expected={"/only_this": "prev_content"},
    ),
    TestCase(
        name="restore_penultimate_when_different_from_previous",
        mode="restore-penultimate",
        spec={
            "CLEAN":       {},
            "PREVIOUS":    {"/prev_version": "pv"},
            "PENULTIMATE": {"/penu_version": "penuv"},
            "LIVE":        {"/live_version": "lv"},
        },
        filter_lines=["- *"],
        expected={"/penu_version": "penuv"},
    ),
    TestCase(
        name="restore_previous_empty_snapshot_yields_empty_volume",
        mode="restore-previous",
        spec={"CLEAN": {"/c": "cv"}, "PREVIOUS": {}, "PENULTIMATE": {"/p": "pv"}, "LIVE": {"/l": "lv"}},
        filter_lines=["- *"],
        expected={},
    ),

    # ======================================================================
    # COMBINED / REGRESSION SCENARIOS
    # ======================================================================

    TestCase(
        name="persistent_file_and_clean_only_file_both_present",
        spec={"CLEAN": {"/persist": "cv", "/clean_only": "cov"}, "PREVIOUS": {"/persist": "uv"}, "PENULTIMATE": {}, "LIVE": {"/persist": "uv"}},
        filter_lines=["+ /persist", "- *"],
        expected={"/persist": "uv", "/clean_only": "cov"},
    ),
    TestCase(
        name="mix_persistent_dirs_and_files_different_roots",
        spec={
            "CLEAN":       {"/.ssh/kh": "ck", "/etc/hosts": "ch"},
            "PREVIOUS":    {"/.ssh/kh": "uk", "/.ssh/id_rsa": "ur", "/etc/hosts": "uh"},
            "PENULTIMATE": {},
            "LIVE":        {"/.ssh/kh": "uk", "/.ssh/id_rsa": "ur", "/etc/hosts": "uh"},
        },
        filter_lines=["+ /.ssh", "+ /.ssh/", "+ /.ssh/**", "- *"],
        expected={"/.ssh": None, "/.ssh/kh": "uk", "/.ssh/id_rsa": "ur", "/etc": None, "/etc/hosts": "ch"},
    ),
    TestCase(
        name="regression_dangling_symlink_with_other_persistent_paths",
        spec={"CLEAN": {}, "PREVIOUS": {"/real": "rv"}, "PENULTIMATE": {}, "LIVE": {"/real": "rv"}},
        filter_lines=["+ /dangling", "+ /real", "- *"],
        post_setup=lambda t: (t["LIVE"] / "dangling").symlink_to("/nonexistent"),
        expected={"/real": "rv", "/dangling": "<unreadable-symlink>"},
    ),
    TestCase(
        name="persistent_paths_some_deleted_some_modified",
        spec={
            "CLEAN":       {"/keep": "ck", "/restore": "cr"},
            "PREVIOUS":    {"/keep": "uk"},
            "PENULTIMATE": {},
            "LIVE":        {"/keep": "uk"},
        },
        filter_lines=["+ /keep", "+ /restore", "- *"],
        expected={"/keep": "uk", "/restore": "cr"},
    ),
    TestCase(
        name="all_three_states_different_content_correct_outcome",
        spec={
            "CLEAN":       {"/f": "clean", "/g": "clean_g"},
            "PREVIOUS":    {"/f": "session"},
            "PENULTIMATE": {"/f": "even_older"},
            "LIVE":        {"/f": "session"},
        },
        filter_lines=["+ /f", "- *"],
        expected={"/f": "session", "/g": "clean_g"},
    ),
    TestCase(
        name="persistent_path_content_unchanged_from_clean_still_preserved",
        spec={"CLEAN": {"/f": "same"}, "PREVIOUS": {"/f": "same"}, "PENULTIMATE": {}, "LIVE": {"/f": "same"}},
        filter_lines=["+ /f", "- *"],
        expected={"/f": "same"},
    ),

    # ======================================================================
    # MULTI-BOOT ROTATION WITH SNAPSHOT VERIFICATION
    # ======================================================================

    TestCase(
        name="multi_boot_persistent_survives_with_rotation",
        spec={"CLEAN": {}, "PREVIOUS": {"/p": "uv"}, "PENULTIMATE": {}, "LIVE": {"/p": "uv"}},
        filter_lines=["+ /p", "- *"],
        expected={"/p": "uv"},
        boot2_live_extra={"/session": "ephemeral_between_boots"},
        expected2={"/p": "uv"},
        check_snapshots2={
            "PREVIOUS":    {"/p": "uv", "/session": "ephemeral_between_boots"},
            "PENULTIMATE": {"/p": "uv"},
        },
    ),

    # ======================================================================
    # PRODUCTION PARITY: READ-ONLY CLEAN SNAPSHOT
    # In production, CLEAN is created as a read-only snapshot (ro=true).
    # btrfs snapshot from ro=true source produces ro=true CURRENT.
    # The binary explicitly runs: btrfs property set -ts CURRENT ro false.
    # Without that line, overlay_persistent would fail to write into CURRENT.
    # ======================================================================

    TestCase(
        name="clean_readonly_reset_works",
        spec={"CLEAN": {"/c": "cv"}, "PREVIOUS": {"/p": "pv"}, "PENULTIMATE": {}, "LIVE": {"/c": "cv", "/p": "pv"}},
        filter_lines=["+ /p", "- *"],
        post_setup=lambda t: subprocess.run(
            ["btrfs", "property", "set", "-ts", str(t["CLEAN"]), "ro", "true"],
            capture_output=True,
        ),
        expected={"/c": "cv", "/p": "pv"},
    ),
    TestCase(
        name="clean_readonly_no_persistent_still_works",
        spec={"CLEAN": {"/sys": "sv", "/etc": "ev"}, "PREVIOUS": {}, "PENULTIMATE": {}, "LIVE": {}},
        filter_lines=["- *"],
        post_setup=lambda t: subprocess.run(
            ["btrfs", "property", "set", "-ts", str(t["CLEAN"]), "ro", "true"],
            capture_output=True,
        ),
        expected={"/sys": "sv", "/etc": "ev"},
    ),

    # ======================================================================
    # FILTER EDGE CASES: WHITESPACE
    # ======================================================================

    TestCase(
        name="filter_trailing_whitespace_on_path_line_trimmed",
        spec={"CLEAN": {"/p": "cv"}, "PREVIOUS": {"/p": "pv"}, "PENULTIMATE": {}, "LIVE": {"/p": "pv"}},
        filter_lines=["+ /p   ", "- *"],
        expected={"/p": "pv"},
    ),
    TestCase(
        name="filter_blank_and_space_only_lines_ignored",
        spec={"CLEAN": {"/c": "cv"}, "PREVIOUS": {"/p": "pv"}, "PENULTIMATE": {}, "LIVE": {"/p": "pv"}},
        filter_lines=["", "   ", "\t", "- *"],
        expected={"/c": "cv"},
    ),

    # ======================================================================
    # SCALE: HUNDREDS OF FILES
    # ======================================================================

    TestCase(
        name="scale_200_non_persistent_files_all_reset",
        spec={
            "CLEAN":       {f"/f{i}": f"c{i}" for i in range(200)},
            "PREVIOUS":    {f"/f{i}": f"s{i}" for i in range(200)},
            "PENULTIMATE": {},
            "LIVE":        {f"/f{i}": f"s{i}" for i in range(200)},
        },
        filter_lines=["- *"],
        expected={f"/f{i}": f"c{i}" for i in range(200)},
    ),
    TestCase(
        name="scale_200_non_persistent_3_persistent_exact_selection",
        spec={
            "CLEAN":       {f"/f{i}": f"c{i}" for i in range(200)},
            "PREVIOUS":    {f"/f{i}": f"c{i}" for i in range(200)} | {"/persist1": "pv1", "/persist2": "pv2", "/persist3": "pv3"},
            "PENULTIMATE": {},
            "LIVE":        {f"/f{i}": f"c{i}" for i in range(200)} | {"/persist1": "pv1", "/persist2": "pv2", "/persist3": "pv3"},
        },
        filter_lines=["+ /persist1", "+ /persist2", "+ /persist3", "- *"],
        expected={f"/f{i}": f"c{i}" for i in range(200)} | {"/persist1": "pv1", "/persist2": "pv2", "/persist3": "pv3"},
    ),
    TestCase(
        name="scale_persistent_dir_100_files_all_preserved",
        spec={
            "CLEAN":       {"/.data/base": "bv"},
            "PREVIOUS":    {"/.data/base": "bv"} | {f"/.data/f{i}": f"uv{i}" for i in range(100)},
            "PENULTIMATE": {},
            "LIVE":        {"/.data/base": "bv"} | {f"/.data/f{i}": f"uv{i}" for i in range(100)},
        },
        filter_lines=["+ /.data", "+ /.data/", "+ /.data/**", "- *"],
        expected={"/.data": None, "/.data/base": "bv"} | {f"/.data/f{i}": f"uv{i}" for i in range(100)},
    ),
]

# ---------------------------------------------------------------------------
# Standalone: multi-subvolume parallel processing
# ---------------------------------------------------------------------------

def run_multi_subvolume_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    name = "multi_subvolume_parallel"

    home_filter = tmpdir / "filter-multi-home"
    root_filter = tmpdir / "filter-multi-root"
    home_filter.write_text("+ /home_persist\n- *\n")
    root_filter.write_text("+ /root_persist\n- *\n")

    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))

    for vol in (VOL, VOL2):
        for subvol in (CLEAN_NAME, "PREVIOUS", "PENULTIMATE", "CURRENT"):
            path = env.mnt / SNAPSHOTS / vol / subvol
            if path.exists():
                Shell.run("btrfs", "subvolume", "delete", str(path))
            Shell.run("btrfs", "subvolume", "create", str(path))
        live = env.mnt / vol
        for child in live.iterdir():
            shutil.rmtree(str(child)) if child.is_dir() else child.unlink()

    for vol, vdata in {
        VOL:  {VOL: {"/home_persist": "hp", "/home_session": "hs"}},
        VOL2: {VOL2: {"/root_persist": "rp", "/root_session": "rs"}},
    }.items():
        for dest_name, files in vdata.items():
            root = env.mnt / dest_name
            for rel, content in files.items():
                dest = root / rel.lstrip("/")
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_text(content)

    Shell.run("umount", str(env.mnt))

    proc = subprocess.run(
        [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "reset",
         f"{VOL}=/home:{home_filter}", f"{VOL2}=/root:{root_filter}"],
        capture_output=True, text=True
    )
    if proc.returncode != 0:
        print(f"  FAIL  {name} (binary exited {proc.returncode})")
        print(textwrap.indent(proc.stdout + proc.stderr, "    "))
        results.failed.append(name)
        return

    home_ok = results.check(f"{name}/@home", env.read_live(VOL), {"/home_persist": "hp"})
    root_ok = results.check(f"{name}/@root", env.read_live(VOL2), {"/root_persist": "rp"})
    if home_ok and root_ok:
        results.passed.remove(f"{name}/@home")
        results.passed.remove(f"{name}/@root")
        results.passed.append(name)
        print(f"  PASS  {name}")

# ---------------------------------------------------------------------------
# Standalone: three consecutive resets
# ---------------------------------------------------------------------------

def run_three_boot_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    name = "three_consecutive_resets_state_stable"
    filter_file = tmpdir / "filter-three-boot"
    filter_file.write_text("+ /persist\n- *\n")

    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))
    for subvol in (CLEAN_NAME, "PREVIOUS", "PENULTIMATE", "CURRENT"):
        path = env.mnt / SNAPSHOTS / VOL / subvol
        if path.exists(): Shell.run("btrfs", "subvolume", "delete", str(path))
        Shell.run("btrfs", "subvolume", "create", str(path))
    live = env.mnt / VOL
    for child in live.iterdir():
        shutil.rmtree(str(child)) if child.is_dir() else child.unlink()
    (live / "persist").write_text("user_data")
    (live / "session").write_text("ephemeral")
    Shell.run("umount", str(env.mnt))

    args = [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "reset",
            f"{VOL}=/home:{filter_file}"]

    for boot_num in range(1, 4):
        proc = subprocess.run(args, capture_output=True, text=True)
        if proc.returncode != 0:
            print(f"  FAIL  {name} (boot {boot_num} binary exited {proc.returncode})")
            print(textwrap.indent(proc.stdout + proc.stderr, "    "))
            results.failed.append(name)
            return
        actual = env.read_live(VOL)
        ok = results.check(f"{name}_boot{boot_num}", actual, {"/persist": "user_data"})
        if not ok:
            return
        results.passed.remove(f"{name}_boot{boot_num}")

    # Also verify PENULTIMATE after 3 boots = post-boot-1 state = just /persist
    snap_penu = env.read_snapshot(VOL, "PENULTIMATE")
    ok = results.check(f"{name}_penultimate", snap_penu, {"/persist": "user_data"})
    if ok:
        results.passed.remove(f"{name}_penultimate")

    results.passed.append(name)
    print(f"  PASS  {name}")

# ---------------------------------------------------------------------------
# Helpers shared by expected-failure standalones
# ---------------------------------------------------------------------------

def _reset_vol(env: BtrfsEnv):
    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))
    for subvol in (CLEAN_NAME, "PREVIOUS", "PENULTIMATE", "CURRENT"):
        path = env.mnt / SNAPSHOTS / VOL / subvol
        if path.exists():
            Shell.run("btrfs", "subvolume", "delete", str(path))
        Shell.run("btrfs", "subvolume", "create", str(path))
    live = env.mnt / VOL
    for child in live.iterdir():
        shutil.rmtree(str(child)) if child.is_dir() else child.unlink()
    Shell.run("umount", str(env.mnt))

def _expect_fail(binary: Path, args: list, name: str, results) -> None:
    proc = subprocess.run(args, capture_output=True, text=True)
    if proc.returncode != 0:
        results.passed.append(name)
        print(f"  PASS  {name}")
    else:
        results.failed.append(name)
        print(f"  FAIL  {name} (expected non-zero exit but got 0)")

# ---------------------------------------------------------------------------
# Standalone: CRLF filter file
# ---------------------------------------------------------------------------

def run_crlf_filter_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    name = "crlf_filter_file_parsed_correctly"
    filter_file = tmpdir / "filter-crlf"
    filter_file.write_bytes(b"+ /p\r\n- *\r\n")  # Windows CRLF

    _reset_vol(env)
    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))
    (env.mnt / SNAPSHOTS / VOL / CLEAN_NAME / "c").write_text("cv")
    (env.mnt / SNAPSHOTS / VOL / "PREVIOUS" / "p").write_text("pv")
    (env.mnt / SNAPSHOTS / VOL / "PREVIOUS" / "c").write_text("cv")
    (env.mnt / VOL / "p").write_text("pv")
    (env.mnt / VOL / "c").write_text("cv")
    Shell.run("umount", str(env.mnt))

    proc = subprocess.run(
        [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "reset",
         f"{VOL}=/home:{filter_file}"],
        capture_output=True, text=True,
    )
    if proc.returncode != 0:
        results.failed.append(name)
        print(f"  FAIL  {name} (binary exited {proc.returncode})")
        print(textwrap.indent(proc.stdout + proc.stderr, "    "))
        return
    results.check(name, env.read_live(VOL), {"/c": "cv", "/p": "pv"})

# ---------------------------------------------------------------------------
# Standalone: expected-failure cases (non-zero exit required)
# ---------------------------------------------------------------------------

def run_missing_clean_fails_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    # Also verifies the LIVE-is-last invariant: LIVE content survives any fatal error
    # because LIVE is only modified in the final step of reset().
    name = "missing_clean_causes_failure"
    filter_file = tmpdir / "filter-missing-clean"
    filter_file.write_text("- *\n")

    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))
    for subvol in ("PREVIOUS", "PENULTIMATE", "CURRENT"):
        path = env.mnt / SNAPSHOTS / VOL / subvol
        if path.exists():
            Shell.run("btrfs", "subvolume", "delete", str(path))
        Shell.run("btrfs", "subvolume", "create", str(path))
    clean_path = env.mnt / SNAPSHOTS / VOL / CLEAN_NAME
    if clean_path.exists():
        Shell.run("btrfs", "subvolume", "delete", str(clean_path))
    live = env.mnt / VOL
    for child in live.iterdir():
        shutil.rmtree(str(child)) if child.is_dir() else child.unlink()
    # Plant known content in LIVE — must survive the failed binary run
    (live / "precious").write_text("must_survive")
    Shell.run("umount", str(env.mnt))

    proc = subprocess.run(
        [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "reset",
         f"{VOL}=/home:{filter_file}"],
        capture_output=True, text=True,
    )
    if proc.returncode == 0:
        results.failed.append(name)
        print(f"  FAIL  {name} (expected non-zero exit but got 0)")
        return
    # LIVE must be unchanged — die() happened before final create_snapshot(CURRENT→LIVE)
    actual = env.read_live(VOL)
    if results.check(f"{name}_live_unchanged", actual, {"/precious": "must_survive"}):
        results.passed.remove(f"{name}_live_unchanged")
        results.passed.append(name)
        print(f"  PASS  {name}")
    else:
        results.failed.append(name)

def run_unreadable_filter_fails_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    # Root bypasses file-permission bits, so chmod 000 doesn't work here.
    # Instead pass a directory path — fs::read_to_string returns EISDIR → die().
    # Also verifies LIVE unchanged: die() in parse_persistent_paths happens AFTER
    # PREVIOUS/PENULTIMATE rotation (steps 1-2) but BEFORE create_snapshot(CURRENT→LIVE).
    name = "unreadable_filter_causes_failure"
    filter_dir = tmpdir / "filter-dir-not-file"
    filter_dir.mkdir(exist_ok=True)

    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))
    for subvol in (CLEAN_NAME, "PREVIOUS", "PENULTIMATE", "CURRENT"):
        path = env.mnt / SNAPSHOTS / VOL / subvol
        if path.exists():
            Shell.run("btrfs", "subvolume", "delete", str(path))
        Shell.run("btrfs", "subvolume", "create", str(path))
    live = env.mnt / VOL
    for child in live.iterdir():
        shutil.rmtree(str(child)) if child.is_dir() else child.unlink()
    (live / "precious").write_text("must_survive")
    (env.mnt / SNAPSHOTS / VOL / CLEAN_NAME / "base").write_text("bv")
    Shell.run("umount", str(env.mnt))

    proc = subprocess.run(
        [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "reset",
         f"{VOL}=/home:{filter_dir}"],
        capture_output=True, text=True,
    )
    if proc.returncode == 0:
        results.failed.append(name)
        print(f"  FAIL  {name} (expected non-zero exit but got 0)")
        return
    actual = env.read_live(VOL)
    if results.check(f"{name}_live_unchanged", actual, {"/precious": "must_survive"}):
        results.passed.remove(f"{name}_live_unchanged")
        results.passed.append(name)
        print(f"  PASS  {name}")
    else:
        results.failed.append(name)

def run_unknown_mode_fails_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    name = "unknown_mode_causes_failure"
    filter_file = tmpdir / "filter-unknown-mode"
    filter_file.write_text("- *\n")

    _reset_vol(env)

    _expect_fail(binary,
                 [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "bananas",
                  f"{VOL}=/home:{filter_file}"],
                 name, results)

def run_restore_missing_snapshot_fails_test(binary: Path, env: BtrfsEnv, tmpdir: Path, results) -> None:
    name = "restore_nonexistent_snapshot_causes_failure"
    filter_file = tmpdir / "filter-restore-missing"
    filter_file.write_text("- *\n")

    _reset_vol(env)
    # Remove PREVIOUS so restore-previous has no source
    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
              env.loop_dev, str(env.mnt))
    prev_path = env.mnt / SNAPSHOTS / VOL / "PREVIOUS"
    if prev_path.exists():
        Shell.run("btrfs", "subvolume", "delete", str(prev_path))
    Shell.run("umount", str(env.mnt))

    _expect_fail(binary,
                 [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, "restore-previous",
                  f"{VOL}=/home:{filter_file}"],
                 name, results)

# ---------------------------------------------------------------------------
# Known behavioral differences between old and new binary
# ---------------------------------------------------------------------------

KNOWN_OLD_DIVERGENCES = {
    "persistent_file_deleted_in_session_gets_clean_fallback",
}

# ---------------------------------------------------------------------------
# Results tracker
# ---------------------------------------------------------------------------

class Results:
    def __init__(self, label: str):
        self.label  = label
        self.passed = []
        self.failed = []

    def check(self, test_name: str, actual: dict, expected: dict) -> bool:
        errors = []
        for path, exp_content in expected.items():
            if path not in actual:
                errors.append(f"  MISSING  {path}")
            elif exp_content is not None and actual[path] != exp_content:
                errors.append(f"  WRONG    {path}: got {actual[path]!r}, want {exp_content!r}")
        for path in actual:
            if path not in expected:
                errors.append(f"  EXTRA    {path}")
        if errors:
            base = test_name.split("_boot")[0].split("/@")[0]
            if self.label == "old" and base in KNOWN_OLD_DIVERGENCES:
                self.passed.append(test_name)
                print(f"  PASS  {test_name}  (expected old-binary divergence)")
                return True
            self.failed.append(test_name)
            print(f"  FAIL  {test_name}")
            for e in errors: print(e)
            return False
        self.passed.append(test_name)
        print(f"  PASS  {test_name}")
        return True

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if os.geteuid() != 0:
        print("ERROR: must run as root (needs mount/losetup)")
        sys.exit(1)
    if len(sys.argv) < 2 or sys.argv[1] not in ("old", "new", "both"):
        print(__doc__)
        sys.exit(1)

    which = sys.argv[1]
    prebuilt = {"old": Path("/tmp/immutability_old"), "new": Path("/tmp/immutability_new")}
    for label, path in prebuilt.items():
        if which in (label, "both") and not path.exists():
            print(f"ERROR: pre-compiled binary missing: {path}")
            print('  nix-shell -p rustc --run "rustc --edition 2021 -O -o /tmp/immutability_old scripts/lib/immutability.rs"')
            print('  nix-shell -p rustc --run "rustc --edition 2021 -O -o /tmp/immutability_new scripts/lib/immutability.rs"')
            sys.exit(1)

    with tempfile.TemporaryDirectory(prefix="imm-test-") as tmpdir:
        tmpdir = Path(tmpdir)
        binaries = {}
        if which in ("old", "both"): binaries["old"] = prebuilt["old"]
        if which in ("new", "both"): binaries["new"] = prebuilt["new"]

        env = BtrfsEnv(tmpdir)
        try:
            print("\nSetting up BTRFS environment...")
            env.setup()
            env.add_volume(VOL2)

            for label, binary in binaries.items():
                results = Results(label)
                print(f"\n{'='*60}")
                print(f"Running tests with {label} binary ({len(TESTS)} cases + standalones)")
                print(f"{'='*60}")

                for test in TESTS:
                    filter_file = tmpdir / f"filter-{test.name}"
                    filter_file.write_text(make_filter(test.filter_lines))

                    Shell.run("mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed",
                              env.loop_dev, str(env.mnt))

                    for subvol in (CLEAN_NAME, "PREVIOUS", "PENULTIMATE", "CURRENT"):
                        path = env.mnt / SNAPSHOTS / VOL / subvol
                        if path.exists():
                            Shell.run("btrfs", "subvolume", "delete", str(path))
                        Shell.run("btrfs", "subvolume", "create", str(path))

                    live = env.mnt / VOL
                    for child in live.iterdir():
                        shutil.rmtree(str(child)) if child.is_dir() else child.unlink()

                    targets = {
                        "CLEAN":       env.mnt / SNAPSHOTS / VOL / CLEAN_NAME,
                        "PREVIOUS":    env.mnt / SNAPSHOTS / VOL / "PREVIOUS",
                        "PENULTIMATE": env.mnt / SNAPSHOTS / VOL / "PENULTIMATE",
                        "CURRENT":     env.mnt / SNAPSHOTS / VOL / "CURRENT",
                        "LIVE":        env.mnt / VOL,
                    }

                    for svol_label, files in test.spec.items():
                        root = targets[svol_label]
                        for rel_path, content in files.items():
                            dest = root / rel_path.lstrip("/")
                            dest.parent.mkdir(parents=True, exist_ok=True)
                            if content is None:
                                dest.mkdir(parents=True, exist_ok=True)
                            else:
                                dest.write_text(content)

                    if test.post_setup:
                        test.post_setup(targets)

                    Shell.run("umount", str(env.mnt))

                    def run_binary():
                        return subprocess.run(
                            [str(binary), env.loop_dev, SNAPSHOTS, CLEAN_NAME, test.mode,
                             f"{VOL}=/home:{filter_file}"],
                            capture_output=True, text=True
                        )

                    proc = run_binary()
                    if proc.returncode != 0:
                        print(f"  FAIL  {test.name} (binary exited {proc.returncode})")
                        print(textwrap.indent(proc.stdout + proc.stderr, "    "))
                        results.failed.append(test.name)
                        continue

                    if test.check_sentinel and not env.sentinel_exists(VOL):
                        print(f"  FAIL  {test.name} (.boot-ready not written)")
                        results.failed.append(test.name)
                        continue

                    if test.sentinel_absent and env.sentinel_exists(VOL):
                        print(f"  FAIL  {test.name} (.boot-ready present but should be absent)")
                        results.failed.append(test.name)
                        continue

                    actual = env.read_live(VOL)
                    if not results.check(test.name, actual, test.expected):
                        continue

                    if test.check_snapshots:
                        snap_ok = True
                        for snap_name, exp_snap in test.check_snapshots.items():
                            if not results.check(f"{test.name}@{snap_name}",
                                                 env.read_snapshot(VOL, snap_name), exp_snap):
                                snap_ok = False
                        if not snap_ok:
                            continue

                    if test.expected2 is not None:
                        if test.boot2_live_extra:
                            env.write_to_live(VOL, test.boot2_live_extra)
                        proc2 = run_binary()
                        if proc2.returncode != 0:
                            print(f"  FAIL  {test.name}_boot2 (binary exited {proc2.returncode})")
                            print(textwrap.indent(proc2.stdout + proc2.stderr, "    "))
                            results.failed.append(f"{test.name}_boot2")
                            continue
                        if not results.check(f"{test.name}_boot2",
                                             env.read_live(VOL), test.expected2):
                            continue
                        if test.check_snapshots2:
                            for snap_name, exp_snap in test.check_snapshots2.items():
                                results.check(f"{test.name}_boot2@{snap_name}",
                                              env.read_snapshot(VOL, snap_name), exp_snap)

                run_multi_subvolume_test(binary, env, tmpdir, results)
                run_three_boot_test(binary, env, tmpdir, results)
                run_crlf_filter_test(binary, env, tmpdir, results)
                run_missing_clean_fails_test(binary, env, tmpdir, results)
                run_unreadable_filter_fails_test(binary, env, tmpdir, results)
                run_unknown_mode_fails_test(binary, env, tmpdir, results)
                run_restore_missing_snapshot_fails_test(binary, env, tmpdir, results)

                print(f"\n{len(results.passed)} passed, {len(results.failed)} failed")
                if results.failed:
                    sys.exit(1)

        finally:
            env.teardown()

if __name__ == "__main__":
    main()
