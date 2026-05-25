#!/usr/bin/env python3
"""
Executable behavior spec for the next immutabilityv2 converge design.

This file intentionally models the desired Btrfs-native behavior before the Rust
implementation is rewritten around it. It does not touch the real filesystem or
mount anything. The fake filesystem is the database: subvolume names, UUID
lineage, readonly flags, and file payloads are enough to decide what converge
should do.

Usage:
  python3 scripts/lib/test/immutabilityv2_converge_spec_test.py
"""
from __future__ import annotations

import random
import sys
from dataclasses import dataclass, field
from pathlib import PurePosixPath

GENERATED_CASES = 900
PERSIST = "@persist"
SNAPSHOTS = "@snapshots"
STAGING = "@staging"
TRASH = "@trash"
CLEAN = "CLEAN"
READY = "READY"
NEXT = "NEXT"
CAPTURE = "CAPTURE"
KEEP = ("A", "B", "C")


class PowerLoss(RuntimeError):
    pass


@dataclass
class Volume:
    name: str
    mount: str
    reset: bool = True


@dataclass
class KeepPath:
    abs_path: str
    kind: str = "auto"


@dataclass
class Plan:
    volume: Volume
    abs_path: str
    rel: str
    kind: str
    subvol: str


@dataclass
class Subvol:
    uuid: int
    parent_uuid: int | None = None
    readonly: bool = False
    files: dict[str, str] = field(default_factory=dict)
    dirs: set[str] = field(default_factory=set)

    def clone(self, uuid: int, readonly: bool | None = None) -> "Subvol":
        return Subvol(
            uuid=uuid,
            parent_uuid=self.uuid,
            readonly=self.readonly if readonly is None else readonly,
            files=dict(self.files),
            dirs=set(self.dirs),
        )


class FakeBtrfs:
    def __init__(self):
        self.next_uuid = 1
        self.subvols: dict[str, Subvol] = {}
        self.ops: list[str] = []

    def exists(self, name: str) -> bool:
        return name in self.subvols

    def subvol(self, name: str) -> Subvol:
        if name not in self.subvols:
            raise AssertionError(f"missing subvolume {name}")
        return self.subvols[name]

    def create(self, name: str, readonly=False) -> None:
        if name in self.subvols:
            raise AssertionError(f"subvolume already exists: {name}")
        self.ops.append(f"create:{name}")
        self.subvols[name] = Subvol(self._uuid(), readonly=readonly)

    def snapshot(self, source: str, dest: str, readonly=False) -> None:
        if source not in self.subvols:
            raise AssertionError(f"snapshot source missing: {source}")
        if dest in self.subvols:
            self.delete(dest)
        self.ops.append(f"snapshot:{source}->{dest}")
        self.subvols[dest] = self.subvols[source].clone(self._uuid(), readonly=readonly)

    def move(self, source: str, dest: str) -> None:
        if source not in self.subvols:
            raise AssertionError(f"move source missing: {source}")
        if dest in self.subvols:
            self.delete(dest)
        self.ops.append(f"move:{source}->{dest}")
        self.subvols[dest] = self.subvols.pop(source)

    def delete(self, name: str) -> None:
        if name not in self.subvols:
            return
        self.ops.append(f"delete:{name}")
        del self.subvols[name]

    def delete_prefix(self, prefix: str, budget: int | None = None) -> int:
        deleted = 0
        for name in sorted(list(self.subvols)):
            if name == prefix or name.startswith(prefix + "/"):
                if budget is not None and deleted >= budget:
                    break
                self.delete(name)
                deleted += 1
        return deleted

    def write(self, subvol: str, rel: str, payload: str) -> None:
        rel = norm_rel(rel)
        self.ops.append(f"write:{subvol}:{rel}")
        self.subvol(subvol).files[rel] = payload
        self._parents(subvol, rel)

    def remove_file(self, subvol: str, rel: str) -> None:
        rel = norm_rel(rel)
        self.ops.append(f"remove-file:{subvol}:{rel}")
        self.subvol(subvol).files.pop(rel, None)

    def mkdir(self, subvol: str, rel: str) -> None:
        rel = norm_rel(rel)
        if not rel:
            return
        self.ops.append(f"mkdir:{subvol}:{rel}")
        self.subvol(subvol).dirs.add(rel)
        self._parents(subvol, rel)

    def file(self, subvol: str, rel: str) -> str | None:
        return self.subvol(subvol).files.get(norm_rel(rel))

    def has_file(self, subvol: str, rel: str) -> bool:
        return norm_rel(rel) in self.subvol(subvol).files

    def has_dir(self, subvol: str, rel: str) -> bool:
        rel = norm_rel(rel)
        sv = self.subvol(subvol)
        return (
            rel in sv.dirs
            or any(path.startswith(rel + "/") for path in sv.files)
            or any(path.startswith(rel + "/") for path in sv.dirs)
        )

    def copy_dir_contents(self, source: str, rel: str, dest: str) -> None:
        rel = norm_rel(rel)
        self.ops.append(f"copy-dir:{source}:{rel}->{dest}")
        src = self.subvol(source)
        dst = self.subvol(dest)
        prefix = rel + "/" if rel else ""
        for path, payload in src.files.items():
            if path == rel:
                continue
            if not rel or path.startswith(prefix):
                dst.files[path[len(prefix):]] = payload
        for path in src.dirs:
            if path == rel:
                continue
            if not rel or path.startswith(prefix):
                dst.dirs.add(path[len(prefix):])

    def copy_file(self, source: str, rel: str, dest: str, dest_rel: str) -> bool:
        rel = norm_rel(rel)
        dest_rel = norm_rel(dest_rel)
        payload = self.file(source, rel)
        if payload is None:
            return False
        self.ops.append(f"copy-file:{source}:{rel}->{dest}:{dest_rel}")
        self.write(dest, dest_rel, payload)
        return True

    def _uuid(self) -> int:
        value = self.next_uuid
        self.next_uuid += 1
        return value

    def _parents(self, subvol: str, rel: str) -> None:
        parts = rel.split("/")[:-1]
        for index in range(len(parts)):
            self.subvol(subvol).dirs.add("/".join(parts[:index + 1]))


class Converger:
    def __init__(self, fs: FakeBtrfs, volumes: list[Volume], keeps: list[KeepPath], publish_live=True, trash_delete_budget=0, crash_at: str | None = None):
        self.fs = fs
        self.volumes = volumes
        self.keeps = keeps
        self.publish_live = publish_live
        self.trash_delete_budget = trash_delete_budget
        self.crash_at = crash_at
        self.crashed = False
        self.mounts: list[tuple[str, str]] = []

    def converge(self) -> list[tuple[str, str]]:
        self.ensure_namespaces()
        self.repair_missing_live()
        self.cleanup_staging()
        plans = self.plan()
        self.mounts = [(plan.abs_path, plan.subvol) for plan in plans if plan.kind == "directory"]
        for volume in self.reset_volumes():
            self.converge_volume(volume, [plan for plan in plans if plan.volume.name == volume.name])
        self.delete_obsolete_persists(plans)
        self.delete_stale_file_payloads(plans)
        self.delete_trash()
        return self.mounts

    def ensure_namespaces(self) -> None:
        for name in (PERSIST, SNAPSHOTS, STAGING, TRASH, f"{PERSIST}/.immutabilityv2/files"):
            if not self.fs.exists(name):
                self.fs.create(name)
        self.checkpoint("after_namespaces")

    def repair_missing_live(self) -> None:
        for volume in self.reset_volumes():
            if self.fs.exists(volume.name):
                continue
            next_name = self.next_name(volume)
            if self.fs.exists(next_name):
                self.fs.move(next_name, volume.name)
                continue
            latest = self.keep_name(volume, "A")
            if self.fs.exists(latest):
                self.fs.snapshot(latest, volume.name)
                continue
            raise AssertionError(f"cannot repair missing live volume {volume.name}")

    def cleanup_staging(self) -> None:
        self.fs.delete_prefix(STAGING)
        if not self.fs.exists(STAGING):
            self.fs.create(STAGING)
        self.checkpoint("after_stale_staging_cleanup")

    def converge_volume(self, volume: Volume, plans: list[Plan]) -> None:
        capture = self.capture_name(volume)
        self.fs.snapshot(volume.name, capture, readonly=True)
        self.checkpoint("after_capture")
        self.ensure_persist_dirs(capture, volume, plans)
        self.save_files(capture, plans)
        self.ensure_ready(volume, plans)
        self.fs.snapshot(self.ready_name(volume), self.next_name(volume), readonly=False)
        self.checkpoint("after_next_snapshot")
        self.restore_files(self.next_name(volume), plans)
        self.rotate_retention(volume, capture)
        self.publish_next(volume)

    def ensure_persist_dirs(self, capture: str, volume: Volume, plans: list[Plan]) -> None:
        clean = self.clean_name(volume)
        for plan in plans:
            if plan.kind != "directory":
                continue
            if self.fs.exists(plan.subvol):
                continue
            stage = f"{STAGING}/persist/{safe(plan.subvol)}"
            self.fs.create(stage)
            self.checkpoint("after_persist_stage_create")
            if self.fs.has_dir(capture, plan.rel):
                self.fs.copy_dir_contents(capture, plan.rel, stage)
            elif self.fs.has_dir(clean, plan.rel):
                self.fs.copy_dir_contents(clean, plan.rel, stage)
            self.checkpoint("after_persist_payload_copy")
            self.fs.move(stage, plan.subvol)
            self.checkpoint("after_persist_publish")

    def save_files(self, capture: str, plans: list[Plan]) -> None:
        for plan in plans:
            if plan.kind != "file":
                continue
            store_rel = file_store_rel(plan.abs_path)
            if not self.fs.copy_file(capture, plan.rel, file_store_name(), store_rel):
                self.fs.remove_file(file_store_name(), store_rel)
            self.checkpoint("after_file_save")

    def ensure_ready(self, volume: Volume, plans: list[Plan]) -> None:
        clean = self.clean_name(volume)
        ready = self.ready_name(volume)
        wanted_dirs = {plan.rel for plan in plans if plan.kind == "directory"}
        if self.ready_valid(clean, ready, wanted_dirs):
            return
        stage = f"{STAGING}/{volume.name}.READY"
        self.fs.snapshot(clean, stage, readonly=False)
        self.checkpoint("after_ready_stage_snapshot")
        for rel in sorted(wanted_dirs):
            self.fs.mkdir(stage, rel)
        self.checkpoint("after_ready_placeholders")
        self.fs.subvol(stage).readonly = True
        self.fs.move(stage, ready)
        self.checkpoint("after_ready_publish")

    def ready_valid(self, clean: str, ready: str, wanted_dirs: set[str]) -> bool:
        if not self.fs.exists(ready):
            return False
        subvol = self.fs.subvol(ready)
        if not subvol.readonly or subvol.parent_uuid != self.fs.subvol(clean).uuid:
            return False
        return all(self.fs.has_dir(ready, rel) for rel in wanted_dirs)

    def restore_files(self, target: str, plans: list[Plan]) -> None:
        for plan in plans:
            if plan.kind != "file":
                continue
            store_rel = file_store_rel(plan.abs_path)
            if self.fs.has_file(file_store_name(), store_rel):
                self.fs.copy_file(file_store_name(), store_rel, target, plan.rel)
            else:
                self.fs.remove_file(target, plan.rel)
            self.checkpoint("after_file_restore")

    def rotate_retention(self, volume: Volume, capture: str) -> None:
        if self.fs.exists(self.keep_name(volume, "C")):
            self.fs.delete(self.keep_name(volume, "C"))
        if self.fs.exists(self.keep_name(volume, "B")):
            self.fs.move(self.keep_name(volume, "B"), self.keep_name(volume, "C"))
        if self.fs.exists(self.keep_name(volume, "A")):
            self.fs.move(self.keep_name(volume, "A"), self.keep_name(volume, "B"))
        self.fs.move(capture, self.keep_name(volume, "A"))
        self.checkpoint("after_retention_rotate")

    def publish_next(self, volume: Volume) -> None:
        if not self.publish_live:
            return
        old = f"{TRASH}/{safe(volume.name)}.{self.fs.subvol(volume.name).uuid}"
        self.fs.move(volume.name, old)
        self.checkpoint("after_live_to_trash")
        self.fs.move(self.next_name(volume), volume.name)
        self.checkpoint("after_next_to_live")

    def delete_obsolete_persists(self, plans: list[Plan]) -> None:
        desired = {plan.subvol for plan in plans if plan.kind == "directory"}
        protected = {PERSIST, file_store_name()}
        for name in sorted(list(self.fs.subvols)):
            if not name.startswith(PERSIST + "/") or name in protected:
                continue
            if name.startswith(file_store_name() + "/"):
                continue
            if name not in desired and name != f"{PERSIST}/.immutabilityv2":
                self.fs.move(name, f"{TRASH}/obsolete.{safe(name)}")
                self.checkpoint("after_obsolete_to_trash")

    def delete_stale_file_payloads(self, plans: list[Plan]) -> None:
        desired = {file_store_rel(plan.abs_path) for plan in plans if plan.kind == "file"}
        store = self.fs.subvol(file_store_name())
        for rel in list(store.files):
            if rel not in desired:
                self.fs.remove_file(file_store_name(), rel)

    def delete_trash(self) -> None:
        deleted = 0
        for name in sorted(list(self.fs.subvols)):
            if not name.startswith(TRASH + "/"):
                continue
            if deleted >= self.trash_delete_budget:
                break
            self.fs.delete(name)
            deleted += 1
        if not self.fs.exists(TRASH):
            self.fs.create(TRASH)
        self.checkpoint("after_trash_delete")

    def plan(self) -> list[Plan]:
        raw: list[Plan] = []
        for keep in self.keeps:
            abs_path, trailing = normalize_abs(keep.abs_path)
            volume = self.volume_for(abs_path)
            if not volume:
                continue
            rel = relative(abs_path, volume.mount)
            kind = self.resolve_kind(volume, rel, keep.kind, trailing)
            raw.append(Plan(volume, abs_path, rel, kind, persist_dir_name(abs_path) if kind == "directory" else file_store_name()))
        raw.sort(key=lambda plan: (plan.volume.name, plan.rel.count("/"), len(plan.rel), plan.rel))
        planned: list[Plan] = []
        for plan in raw:
            if any(existing.volume.name == plan.volume.name and existing.kind == "directory" and ancestor(existing.rel, plan.rel) for existing in planned):
                continue
            planned.append(plan)
        return planned

    def resolve_kind(self, volume: Volume, rel: str, kind: str, trailing: bool) -> str:
        if kind in ("directory", "dir"):
            return "directory"
        if kind == "file":
            return "file"
        if trailing:
            return "directory"
        for source in (volume.name, self.clean_name(volume)):
            if self.fs.exists(source) and self.fs.has_dir(source, rel):
                return "directory"
            if self.fs.exists(source) and self.fs.has_file(source, rel):
                return "file"
        return "file"

    def volume_for(self, abs_path: str) -> Volume | None:
        candidates = [volume for volume in self.reset_volumes() if under_mount(abs_path, volume.mount)]
        if not candidates:
            return None
        return max(candidates, key=lambda volume: len(volume.mount))

    def reset_volumes(self) -> list[Volume]:
        return [volume for volume in self.volumes if volume.reset]

    def clean_name(self, volume: Volume) -> str:
        return f"{SNAPSHOTS}/{volume.name}/{CLEAN}"

    def ready_name(self, volume: Volume) -> str:
        return f"{SNAPSHOTS}/{volume.name}/{READY}"

    def next_name(self, volume: Volume) -> str:
        return f"{SNAPSHOTS}/{volume.name}/{NEXT}"

    def capture_name(self, volume: Volume) -> str:
        return f"{STAGING}/{volume.name}.{CAPTURE}"

    def keep_name(self, volume: Volume, label: str) -> str:
        return f"{SNAPSHOTS}/{volume.name}/{label}"

    def checkpoint(self, name: str) -> None:
        self.fs.ops.append(f"checkpoint:{name}")
        if self.crash_at == name and not self.crashed:
            self.crashed = True
            raise PowerLoss(name)


def normalize_abs(path: str) -> tuple[str, bool]:
    if not path.startswith("/"):
        raise AssertionError(f"path must be absolute: {path}")
    trailing = len(path) > 1 and path.endswith("/")
    return ("/" if path == "/" else path.rstrip("/")), trailing


def norm_rel(path: str) -> str:
    return str(PurePosixPath(path)).strip("/")


def under_mount(path: str, mount: str) -> bool:
    if mount == "/":
        return True
    return path == mount or path.startswith(mount.rstrip("/") + "/")


def relative(path: str, mount: str) -> str:
    if mount == "/":
        return norm_rel(path)
    if path == mount:
        return ""
    return norm_rel(path[len(mount.rstrip("/") + "/"):])


def persist_dir_name(abs_path: str) -> str:
    return f"{PERSIST}/{abs_path.lstrip('/')}"


def file_store_name() -> str:
    return f"{PERSIST}/.immutabilityv2/files"


def file_store_rel(abs_path: str) -> str:
    return abs_path.lstrip("/")


def safe(path: str) -> str:
    return path.strip("/").replace("/", "__")


def ancestor(parent: str, child: str) -> bool:
    return bool(parent) and child.startswith(parent + "/")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def seed_base() -> tuple[FakeBtrfs, list[Volume]]:
    fs = FakeBtrfs()
    volumes = [Volume("@root", "/"), Volume("@home", "/home"), Volume("@var", "/var", reset=False)]
    for name in (SNAPSHOTS,):
        fs.create(name)
    for volume in volumes[:2]:
        fs.create(volume.name)
        fs.create(f"{SNAPSHOTS}/{volume.name}")
        fs.create(f"{SNAPSHOTS}/{volume.name}/{CLEAN}", readonly=True)
    return fs, volumes


def assert_invariants(fs: FakeBtrfs, volumes: list[Volume], keeps: list[KeepPath], mounts: list[tuple[str, str]]) -> None:
    require(fs.exists(PERSIST), "persist root must exist")
    require(fs.exists(file_store_name()), "file payload subvolume must exist")
    for volume in [volume for volume in volumes if volume.reset]:
        require(fs.exists(volume.name), f"live volume missing: {volume.name}")
        require(fs.exists(f"{SNAPSHOTS}/{volume.name}/{CLEAN}"), f"CLEAN missing for {volume.name}")
        require(fs.exists(f"{SNAPSHOTS}/{volume.name}/{READY}"), f"READY missing for {volume.name}")
        require(fs.subvol(f"{SNAPSHOTS}/{volume.name}/{READY}").readonly, "READY must be readonly")
        kept = [name for name in KEEP if fs.exists(f"{SNAPSHOTS}/{volume.name}/{name}")]
        require(len(kept) <= 3, f"too many retained generations for {volume.name}")
    for target, subvol in mounts:
        require(target.startswith("/"), f"mount target must be absolute: {target}")
        require(subvol.startswith(PERSIST + "/"), f"mount subvol must be under persist: {subvol}")
        require(fs.exists(subvol), f"mount subvolume missing: {subvol}")


def run_converge(fs: FakeBtrfs, volumes: list[Volume], keeps: list[KeepPath], **kwargs) -> list[tuple[str, str]]:
    converger = Converger(fs, volumes, keeps, **kwargs)
    mounts = converger.converge()
    assert_invariants(fs, volumes, keeps, mounts)
    return mounts


def workflow_fresh_converge_promotes_dirs_and_files() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/Games/save.dat", "game")
    fs.write("@home", "alex/.bash_history", "history")
    fs.write("@home", "alex/tmp.txt", "discard")
    fs.write("@snapshots/@home/CLEAN", "alex/default.txt", "clean")
    keeps = [KeepPath("/home/alex/Games"), KeepPath("/home/alex/.bash_history")]
    mounts = run_converge(fs, volumes, keeps)
    require(("/home/alex/Games", "@persist/home/alex/Games") in mounts, "direct mount missing")
    require(fs.file("@persist/home/alex/Games", "save.dat") == "game", "persisted dir payload missing")
    require(fs.file("@home", "alex/.bash_history") == "history", "file persist not restored")
    require(fs.file("@home", "alex/tmp.txt") is None, "non-persisted file survived reset")


def workflow_fast_path_skips_directory_copy() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/.local/share/Steam/appmanifest.acf", "steam")
    keeps = [KeepPath("/home/alex/.local/share/Steam")]
    run_converge(fs, volumes, keeps)
    fs.ops.clear()
    fs.write("@home", "alex/runtime-only", "discard")
    run_converge(fs, volumes, keeps)
    require(not any(op.startswith("copy-dir:") for op in fs.ops), "fast path copied a persisted directory")


def workflow_nested_parent_wins() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/.config/kwinrc", "kwin")
    keeps = [KeepPath("/home/alex/.config"), KeepPath("/home/alex/.config/kwinrc")]
    mounts = run_converge(fs, volumes, keeps)
    require(mounts == [("/home/alex/.config", "@persist/home/alex/.config")], f"nested file was not collapsed: {mounts}")
    require(fs.file(file_store_name(), "home/alex/.config/kwinrc") is None, "nested file got dangling payload")


def workflow_userspace_busy_publish_is_completed_by_boot() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/file", "value")
    keeps = [KeepPath("/home/alex/file")]
    Converger(fs, volumes, keeps, publish_live=False).converge()
    require(fs.exists("@snapshots/@home/NEXT"), "userspace publish should leave NEXT when live is busy")
    require(fs.file("@home", "alex/file") == "value", "busy userspace publish should not replace live")
    run_converge(fs, volumes, keeps)
    require(not fs.exists("@snapshots/@home/NEXT"), "boot converge should publish pending NEXT")


def workflow_clean_change_rebuilds_ready() -> None:
    fs, volumes = seed_base()
    keeps = [KeepPath("/home/alex/Downloads/")]
    run_converge(fs, volumes, keeps)
    old_uuid = fs.subvol("@snapshots/@home/READY").uuid
    fs.delete("@snapshots/@home/CLEAN")
    fs.create("@snapshots/@home/CLEAN", readonly=True)
    fs.write("@snapshots/@home/CLEAN", "alex/new-clean", "v2")
    run_converge(fs, volumes, keeps)
    require(fs.subvol("@snapshots/@home/READY").uuid != old_uuid, "READY was not rebuilt after CLEAN changed")


def workflow_obsolete_persist_moves_to_trash() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/Downloads/file", "download")
    run_converge(fs, volumes, [KeepPath("/home/alex/Downloads")])
    run_converge(fs, volumes, [])
    require(not fs.exists("@persist/home/alex/Downloads"), "obsolete persist subvolume remained live")
    require(any(name.startswith("@trash/obsolete.") for name in fs.subvols), "obsolete persist was not moved to trash")


def workflow_stale_file_payload_is_removed() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/file", "one")
    run_converge(fs, volumes, [KeepPath("/home/alex/file")])
    require(fs.file(file_store_name(), "home/alex/file") == "one", "file payload missing")
    run_converge(fs, volumes, [])
    require(fs.file(file_store_name(), "home/alex/file") is None, "stale file payload was not removed")


def workflow_retains_a_b_c_only() -> None:
    fs, volumes = seed_base()
    keeps: list[KeepPath] = []
    for index in range(6):
        fs.write("@home", f"alex/runtime-{index}", str(index))
        run_converge(fs, volumes, keeps)
    retained = [name for name in fs.subvols if name.startswith("@snapshots/@home/") and name.rsplit("/", 1)[-1] in KEEP]
    require(sorted(name.rsplit("/", 1)[-1] for name in retained) == ["A", "B", "C"], f"wrong retained set: {retained}")


def workflow_trash_delete_budget_is_bounded() -> None:
    fs, volumes = seed_base()
    for index in range(10):
        fs.create(f"@trash/old-{index}")
    run_converge(fs, volumes, [], trash_delete_budget=2)
    deleted = [op for op in fs.ops if op.startswith("delete:@trash/")]
    require(len(deleted) == 2, f"trash deletion ignored budget: {deleted}")


def workflow_non_reset_volume_is_ignored() -> None:
    fs, volumes = seed_base()
    fs.create("@var")
    fs.write("@var", "log/app.log", "log")
    run_converge(fs, volumes, [KeepPath("/var/log/app.log")])
    require(fs.file(file_store_name(), "var/log/app.log") is None, "non-reset volume path was file-persisted")


def workflow_missing_file_persists_deletion() -> None:
    fs, volumes = seed_base()
    fs.create("@persist")
    fs.create(file_store_name())
    fs.write(file_store_name(), "home/alex/missing", "old")
    run_converge(fs, volumes, [KeepPath("/home/alex/missing")])
    require(fs.file(file_store_name(), "home/alex/missing") is None, "missing file did not remove old payload")
    require(fs.file("@home", "alex/missing") is None, "missing file was restored")


def workflow_symlink_payload_is_treated_as_file() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/link", "symlink:target")
    run_converge(fs, volumes, [KeepPath("/home/alex/link")])
    require(fs.file("@home", "alex/link") == "symlink:target", "symlink payload changed")


def workflow_spaces_and_special_names_are_stable() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/with space/file name.txt", "value")
    run_converge(fs, volumes, [KeepPath("/home/alex/with space/")])
    require(fs.file("@persist/home/alex/with space", "file name.txt") == "value", "space path did not persist")


def workflow_directory_mounts_are_derived_from_spec() -> None:
    fs, volumes = seed_base()
    fs.write("@home", "alex/Downloads/file", "download")
    mounts = run_converge(fs, volumes, [KeepPath("/home/alex/Downloads")])
    require(mounts == [("/home/alex/Downloads", "@persist/home/alex/Downloads")], "mount list was not derived from spec")


def workflow_existing_persist_is_not_overwritten() -> None:
    fs, volumes = seed_base()
    fs.create("@persist")
    fs.create("@persist/.immutabilityv2/files")
    fs.create("@persist/home/alex/Games")
    fs.write("@persist/home/alex/Games", "old-save", "old")
    fs.write("@home", "alex/Games/new-save", "new")
    run_converge(fs, volumes, [KeepPath("/home/alex/Games")])
    require(fs.file("@persist/home/alex/Games", "old-save") == "old", "existing persist was overwritten")
    require(fs.file("@persist/home/alex/Games", "new-save") is None, "existing persist was recopied")


def run_named_workflows() -> int:
    workflows = [
        workflow_fresh_converge_promotes_dirs_and_files,
        workflow_fast_path_skips_directory_copy,
        workflow_nested_parent_wins,
        workflow_userspace_busy_publish_is_completed_by_boot,
        workflow_clean_change_rebuilds_ready,
        workflow_obsolete_persist_moves_to_trash,
        workflow_stale_file_payload_is_removed,
        workflow_retains_a_b_c_only,
        workflow_trash_delete_budget_is_bounded,
        workflow_non_reset_volume_is_ignored,
        workflow_missing_file_persists_deletion,
        workflow_symlink_payload_is_treated_as_file,
        workflow_spaces_and_special_names_are_stable,
        workflow_directory_mounts_are_derived_from_spec,
        workflow_existing_persist_is_not_overwritten,
    ]
    for workflow in workflows:
        workflow()
    return len(workflows)


def crash_templates() -> list[tuple[str, list[KeepPath], dict[str, str]]]:
    return [
        ("dir", [KeepPath("/home/alex/Games")], {"alex/Games/save": "save"}),
        ("file", [KeepPath("/home/alex/.bash_history")], {"alex/.bash_history": "history"}),
        ("mixed", [KeepPath("/home/alex/Games"), KeepPath("/home/alex/.bash_history")], {"alex/Games/save": "save", "alex/.bash_history": "history"}),
        ("obsolete", [], {}),
    ]


def run_fault_matrix() -> int:
    crash_points = [
        "after_namespaces",
        "after_stale_staging_cleanup",
        "after_capture",
        "after_persist_stage_create",
        "after_persist_payload_copy",
        "after_persist_publish",
        "after_file_save",
        "after_ready_stage_snapshot",
        "after_ready_placeholders",
        "after_ready_publish",
        "after_next_snapshot",
        "after_file_restore",
        "after_retention_rotate",
        "after_live_to_trash",
        "after_next_to_live",
        "after_obsolete_to_trash",
        "after_trash_delete",
    ]
    ran = 0
    for crash_at in crash_points:
        for _, keeps, files in crash_templates():
            fs, volumes = seed_base()
            for rel, payload in files.items():
                fs.write("@home", rel, payload)
            fs.create("@persist/home/alex/old-obsolete")
            try:
                Converger(fs, volumes, keeps, crash_at=crash_at).converge()
            except PowerLoss:
                pass
            run_converge(fs, volumes, keeps)
            ran += 1
    return ran


def generated_case(index: int) -> None:
    rng = random.Random(0xC0A57E + index)
    fs, volumes = seed_base()
    parts = [
        "alex",
        ".config",
        ".local",
        "share",
        "cache",
        "Steam",
        "Games",
        "Code",
        "with space",
        f"case-{index}",
    ]
    keeps: list[KeepPath] = []
    for slot in range(1 + index % 6):
        base = ["alex"] + [rng.choice(parts[1:]) for _ in range(1 + rng.randrange(3))]
        as_dir = rng.randrange(3) == 0
        if as_dir:
            path = "/home/" + "/".join(base) + "/"
            keeps.append(KeepPath(path))
            fs.write("@home", "/".join(base + [f"file-{slot}"]), f"value-{index}-{slot}")
        else:
            path = "/home/" + "/".join(base + [f"file-{slot}"])
            keeps.append(KeepPath(path))
            if rng.randrange(4) != 0:
                fs.write("@home", "/".join(base + [f"file-{slot}"]), f"value-{index}-{slot}")
    if index % 5 == 0:
        keeps.append(KeepPath("/home/alex/.config/"))
        keeps.append(KeepPath("/home/alex/.config/kwinrc"))
        fs.write("@home", "alex/.config/kwinrc", "kwin")
    if index % 7 == 0:
        fs.create(f"@persist/home/alex/obsolete-{index}")
    if index % 11 == 0:
        keeps.append(KeepPath("/var/log/app.log"))
        fs.create("@var")
        fs.write("@var", "log/app.log", "ignored")
    publish_live = index % 13 != 0
    mounts = Converger(fs, volumes, keeps, publish_live=publish_live, trash_delete_budget=index % 3).converge()
    if not publish_live:
        mounts = run_converge(fs, volumes, keeps, trash_delete_budget=index % 3)
    assert_invariants(fs, volumes, keeps, mounts)
    if index % 2 == 0:
        fs.ops.clear()
        run_converge(fs, volumes, keeps)
        require(not any(op.startswith("copy-dir:") for op in fs.ops), f"generated fast path recopied dirs in case {index}")


def run_generated_cases() -> int:
    for index in range(GENERATED_CASES):
        generated_case(index)
    return GENERATED_CASES


def main() -> None:
    total = 0
    total += run_named_workflows()
    total += run_fault_matrix()
    total += run_generated_cases()
    print(f"{total} immutabilityv2 converge behavior scenarios passed")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise
