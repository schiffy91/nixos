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
            subvol = os.path.join(
                os.path.dirname(path), " ".join(parts[8:])
            )
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


def read_paths_file(paths_file):
    with open(paths_file, "r") as f:
        return [line.strip() for line in f if line.strip()]


def build_rsync_filter(mount_point, paths, previous):
    lines = ["+ */"]
    for path in paths:
        if mount_point == "/":
            relative = path.lstrip("/")
        elif path == mount_point or path.startswith(mount_point + "/"):
            relative = path[len(mount_point):].lstrip("/")
        else:
            continue
        if not relative:
            continue
        full = os.path.join(previous, relative)
        if os.path.isdir(full):
            lines.append(f"+ /{relative}/")
            lines.append(f"+ /{relative}/**")
        elif os.path.exists(full):
            lines.append(f"+ /{relative}")
        else:
            log(f"  skip (not in previous): {path}")
    lines.append("- *")
    return lines


def copy_persistent_files(mount_point, paths, previous, current):
    log(f"Preserving persistent paths for mount_point={mount_point}")
    filter_lines = build_rsync_filter(mount_point, paths, previous)
    has_includes = any(
        l.startswith("+ /") and l != "+ */" for l in filter_lines
    )
    if not has_includes:
        log(f"No paths matched mount_point={mount_point}; skipping rsync")
        return
    for line in filter_lines:
        log(f"  filter: {line}")
    filter_path = f"/tmp/immutability-filter-{os.getpid()}"
    with open(filter_path, "w") as f:
        f.write("\n".join(filter_lines) + "\n")
    if run(f"cd {previous} && rsync -aHAX --numeric-ids --delete"
           f" --filter='. {filter_path}' . {current}") != 0:
        abort("rsync failed to copy persistent files")


def reset_subvolume(name, mount_point, snapshots_name, clean_name, paths):
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
    copy_persistent_files(mount_point, paths, previous, current)
    create_sentinel(current)
    btrfs_snapshot(current, subvolume)


def restore_subvolume(name, snapshots_name, snapshot_label):
    subvolume = f"{_mount_point}/{name}"
    snapshots = f"{_mount_point}/{snapshots_name}/{name}"
    source = f"{snapshots}/{snapshot_label}"

    if not os.path.isdir(source):
        abort(f"Cannot restore: {source} does not exist")

    log(f"Restoring {name} from {snapshot_label}")
    btrfs_snapshot(source, subvolume)
    log(f"Restored {name} from {snapshot_label}")


def snapshot_only(name, snapshots_name, clean_name):
    subvolume = f"{_mount_point}/{name}"
    snapshots = f"{_mount_point}/{snapshots_name}/{name}"
    clean = f"{snapshots}/{clean_name}"
    previous = f"{snapshots}/PREVIOUS"
    penultimate = f"{snapshots}/PENULTIMATE"

    require(f"-n {clean}")
    require(f"-d {clean}")
    if not os.path.isdir(penultimate):
        btrfs_snapshot(clean, penultimate)
    if not os.path.isdir(previous):
        btrfs_snapshot(clean, previous)

    btrfs_snapshot(previous, penultimate)
    btrfs_snapshot(subvolume, previous)
    log(f"Snapshot-only complete for {name} (no wipe)")


def main():
    if len(sys.argv) < 6:
        error(f"Usage: {sys.argv[0]} <device> <snapshots_name>"
              " <clean_name> <mode> <paths_file> [name=mount ...]")
        sys.exit(1)

    device = sys.argv[1]
    snapshots_name = sys.argv[2]
    clean_name = sys.argv[3]
    mode = sys.argv[4]
    paths_file = sys.argv[5]
    pairs = sys.argv[6:]
    subvolume_names = [p.split("=")[0] for p in pairs]

    log(f"Mode={mode} device={device}"
        f" subvolumes={' '.join(subvolume_names)}")
    mount_subvolumes(device)

    if mode == "disabled":
        log("Immutability disabled; skipping all operations")
        unmount_subvolumes()
        return

    paths = read_paths_file(paths_file)

    for pair in pairs:
        parts = pair.split("=", 1)
        name = parts[0]
        mount_point = parts[1] if len(parts) > 1 else "/"

        if mode == "reset":
            log(f"Resetting {name} (mount_point={mount_point})")
            reset_subvolume(
                name, mount_point, snapshots_name, clean_name, paths,
            )
        elif mode == "snapshot-only":
            log(f"Snapshot-only {name}")
            snapshot_only(name, snapshots_name, clean_name)
        elif mode in ("restore-previous", "restore-penultimate"):
            label = mode.split("-", 1)[1].upper()
            log(f"Restoring {name} from {label}")
            restore_subvolume(name, snapshots_name, label)
        else:
            abort(f"Unknown mode: {mode}")

    unmount_subvolumes()
    log("Immutability complete")


if __name__ == "__main__":
    main()
