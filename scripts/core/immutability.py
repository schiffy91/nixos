import os
import sys
import subprocess

_depth = 0
_mount_point = "/mnt"


def log(msg):
    print("  " * _depth + msg)


def warn(msg):
    print("  " * _depth + f"WRN {msg}", file=sys.stderr)


def error(msg):
    print("  " * _depth + f"ERR {msg}", file=sys.stderr)


def run(cmd):
    global _depth
    _depth += 1
    log(cmd)
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    for line in result.stdout.splitlines():
        if line.strip():
            log(line)
    if result.returncode != 0:
        for line in result.stderr.splitlines():
            if line.strip():
                warn(line)
    _depth -= 1
    return result.returncode


def abort(msg):
    error(msg)
    error("Unmounting and quitting")
    run(f"umount -R {_mount_point}")
    run(f"rm -rf {_mount_point}")
    sys.exit(1)


def require(test_args):
    if run(f"test {test_args}") != 0:
        abort(f"Require failed: test {test_args}")


def btrfs_sync(path):
    run(f"btrfs filesystem sync {path}")


def btrfs_delete(path):
    if run(f"btrfs subvolume delete {path} --commit-after") != 0:
        abort(f"Failed to delete {path}")
    btrfs_sync(os.path.dirname(path))


def btrfs_delete_recursively(path):
    if not os.path.isdir(path):
        return
    result = subprocess.run(
        f"btrfs subvolume list -o {path}",
        shell=True, capture_output=True, text=True,
    )
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 9:
            subvol = os.path.join(os.path.dirname(path), parts[8])
            btrfs_delete_recursively(subvol)
    btrfs_delete(path)


def btrfs_snapshot(src, dst):
    require(f"-d {src}")
    btrfs_delete_recursively(dst)
    if run(f"btrfs subvolume snapshot {src} {dst}") != 0:
        abort(f"Failed to snapshot {src} to {dst}")
    btrfs_sync(src)
    btrfs_sync(dst)


def btrfs_set_rw(path):
    if run(f"btrfs property set -ts {path} ro false") != 0:
        abort(f"Failed to make {path} read-write")


def mount_subvolumes(disk):
    require(f"-b {disk}")
    run(f"mkdir -p {_mount_point}")
    run(f"mount -t btrfs -o subvolid=5,user_subvol_rm_allowed"
        f" {disk} {_mount_point}")


def unmount_subvolumes():
    run(f"umount {_mount_point}")
    run(f"rm -rf {_mount_point}")


def check_recovery_needed(snapshot):
    return (os.path.isdir(snapshot)
            and not os.path.isfile(f"{snapshot}/.boot-ready"))


def create_sentinel(snapshot):
    with open(f"{snapshot}/.boot-ready", "w"):
        pass
    btrfs_sync(snapshot)


def copy_persistent_files(previous, current, filter_file):
    log("Preserving persistent paths (precomputed filter)")
    if run(f"cd {previous} && rsync -aHAX --numeric-ids --delete"
           f" --filter='. {filter_file}' . {current}") != 0:
        abort("rsync failed to copy persistent files")


def reset_subvolume(name, snapshots_name, clean_name, filter_file):
    subvolume = f"{_mount_point}/{name}"
    snapshots = f"{_mount_point}/{snapshots_name}/{name}"
    clean = f"{snapshots}/{clean_name}"
    previous = f"{snapshots}/PREVIOUS"
    penultimate = f"{snapshots}/PENULTIMATE"
    current = f"{snapshots}/CURRENT"

    if check_recovery_needed(current):
        warn("Incomplete boot detected (missing .boot-ready sentinel)")
        btrfs_delete_recursively(current)

    require(f"-n {clean}")
    require(f"-d {clean}")
    if not os.path.isdir(penultimate):
        btrfs_snapshot(clean, penultimate)
    if not os.path.isdir(previous):
        btrfs_snapshot(clean, previous)

    btrfs_snapshot(previous, penultimate)
    btrfs_snapshot(subvolume, previous)
    btrfs_snapshot(clean, current)
    btrfs_set_rw(current)
    copy_persistent_files(previous, current, filter_file)
    create_sentinel(current)
    btrfs_snapshot(current, subvolume)


def main():
    if len(sys.argv) < 5:
        error(f"Usage: {sys.argv[0]} <device> <snapshots_name>"
              " <clean_name> <filter_file> [name=mount ...]")
        sys.exit(1)

    device = sys.argv[1]
    snapshots_name = sys.argv[2]
    clean_name = sys.argv[3]
    filter_file = sys.argv[4]
    pairs = sys.argv[5:]
    subvolume_names = [p.split("=")[0] for p in pairs]

    log(f"Mounting {device} subvolumes={' '.join(subvolume_names)}")
    mount_subvolumes(device)

    for pair in pairs:
        name = pair.split("=")[0]
        log(f"Resetting {name}")
        reset_subvolume(name, snapshots_name, clean_name, filter_file)

    unmount_subvolumes()
    log("Immutability reset complete")


if __name__ == "__main__":
    main()
