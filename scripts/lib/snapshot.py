import os, shlex

from .shell import Shell, chrootable
from .utils import Utils
from .config import Config

@chrootable
class Snapshot:
    sh = Shell()
    @classmethod
    def get_snapshots_path(cls):
        return Config.eval(
            "config.settings.disk.subvolumes.snapshots.mountPoint")
    @classmethod
    def get_snapshots_subvolume_name(cls):
        return Config.eval(
            "config.settings.disk.subvolumes.snapshots.name")
    @classmethod
    def get_clean_snapshot_name(cls):
        return Config.eval(
            "config.settings.disk.immutability.persist.snapshots.cleanName")
    @classmethod
    def get_subvolumes_to_reset_on_boot(cls):
        raw = Config.eval(
            "config.settings.disk.subvolumes."
            "nameMountPointPairs.resetOnBoot")
        return [pair.split("=") for pair in str(raw).split()]
    @classmethod
    def get_clean_snapshot_path(cls, subvolume_name):
        return (f"{cls.get_snapshots_path()}"
                f"/{subvolume_name}"
                f"/{cls.get_clean_snapshot_name()}")
    @classmethod
    def is_subvolume(cls, path):
        if not cls.sh.exists(path): return False
        return cls.sh.run(f"btrfs subvolume show {shlex.quote(path)}",
                          check=False).returncode == 0
    @classmethod
    def is_readonly(cls, path):
        result = cls.sh.run(
            f"btrfs property get -ts {shlex.quote(path)} ro", check=False)
        return result.returncode == 0 and "ro=true" in Shell.stdout(result)
    @classmethod
    def child_subvolumes(cls, path):
        result = cls.sh.run(f"btrfs subvolume list -o {shlex.quote(path)}",
                            check=False)
        if result.returncode != 0:
            raise RuntimeError(f"Failed to list child subvolumes under {path}")
        lines = Shell.stdout(result).splitlines()
        paths = []
        snapshots_name = cls.get_snapshots_subvolume_name()
        snapshots_path = cls.get_snapshots_path()
        for line in lines:
            if " path " not in line: continue
            child = line.split(" path ", 1)[1]
            if child == snapshots_name:
                paths.append(snapshots_path)
            elif child.startswith(f"{snapshots_name}/"):
                paths.append(f"{snapshots_path}/{child[len(snapshots_name) + 1:]}")
            else:
                paths.append(f"/{child}")
        return sorted(paths, key=lambda p: (p.count("/"), len(p)), reverse=True)
    @classmethod
    def delete_subvolume(cls, path):
        if not cls.sh.exists(path): return
        if not cls.is_subvolume(path):
            raise RuntimeError(f"Refusing to delete non-subvolume: {path}")
        for child in cls.child_subvolumes(path):
            cls.delete_subvolume(child)
        cls.sh.run(f"btrfs subvolume delete -C {shlex.quote(path)}")
    @classmethod
    def create_initial_snapshot(cls, name, mount_point):
        clean_path = cls.get_clean_snapshot_path(name)
        temporary_path = f"{clean_path}.tmp.{os.getpid()}"
        cls.delete_subvolume(temporary_path)
        cls.sh.mkdir(cls.sh.dirname(clean_path))
        cls.sh.run(
            f"btrfs subvolume snapshot -r "
            f"{shlex.quote(mount_point)} {shlex.quote(temporary_path)}")
        if not cls.is_subvolume(temporary_path):
            raise RuntimeError(f"CLEAN temporary is not a subvolume: {temporary_path}")
        if not cls.is_readonly(temporary_path):
            raise RuntimeError(f"CLEAN temporary is not readonly: {temporary_path}")
        parent = cls.sh.dirname(clean_path)
        cls.delete_subvolume(f"{parent}/READY")
        cls.delete_subvolume(f"{parent}/NEXT")
        if cls.sh.exists(clean_path):
            if not cls.is_subvolume(clean_path):
                raise RuntimeError(f"Refusing to replace non-subvolume CLEAN: {clean_path}")
            cls.sh.run(
                f"mv -T --exchange --no-copy {shlex.quote(temporary_path)} "
                f"{shlex.quote(clean_path)}")
            cls.delete_subvolume(temporary_path)
        else:
            cls.sh.run(
                f"mv --no-copy {shlex.quote(temporary_path)} "
                f"{shlex.quote(clean_path)}")
        if not cls.is_subvolume(clean_path):
            raise RuntimeError(f"CLEAN is not a subvolume: {clean_path}")
        if not cls.is_readonly(clean_path):
            raise RuntimeError(f"CLEAN is not readonly: {clean_path}")
    @classmethod
    def create_initial_snapshots(cls):
        for name, mount_point in cls.get_subvolumes_to_reset_on_boot():
            try:
                cls.create_initial_snapshot(name, mount_point)
            except Exception as e:
                temporary_path = (
                    f"{cls.get_clean_snapshot_path(name)}.tmp.{os.getpid()}")
                try: cls.delete_subvolume(temporary_path)
                except Exception: pass
                Utils.log_error(f"Failed to create a clean snapshot for {name}\n{e}")
                raise
