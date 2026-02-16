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
    def create_initial_snapshots(cls):
        for name, mount_point in cls.get_subvolumes_to_reset_on_boot():
            clean_path = cls.get_clean_snapshot_path(name)
            try:
                cls.sh.rm(clean_path)
                cls.sh.mkdir(cls.sh.dirname(clean_path))
                cls.sh.run(
                    f"btrfs subvolume snapshot -r {mount_point} {clean_path}")
            except BaseException as e:
                Utils.log_error(
                    f"Failed to create a clean snapshot for {name}\n{e}")
