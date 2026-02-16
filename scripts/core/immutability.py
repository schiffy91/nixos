import os
import sys
import subprocess
import threading

sys.stdout = os.fdopen(sys.stdout.fileno(), "w", buffering=1, closefd=False)
sys.stderr = os.fdopen(sys.stderr.fileno(), "w", buffering=1, closefd=False)

_mount_point = "/mnt"


def log(msg):
    print(msg, flush=True)


def warn(msg):
    print(f"WRN {msg}", file=sys.stderr, flush=True)


def error(msg):
    print(f"ERR {msg}", file=sys.stderr, flush=True)


def run(cmd):
    display = cmd if isinstance(cmd, str) else " ".join(cmd)
    log(f"  {display}")
    args = cmd.split() if isinstance(cmd, str) else cmd
    result = subprocess.run(args, capture_output=True, text=True, check=False)
    for line in result.stdout.splitlines():
        if line.strip():
            log(f"  {line}")
    if result.returncode != 0:
        for line in result.stderr.splitlines():
            if line.strip():
                warn(line)
    return result.returncode


def abort(msg):
    error(msg)
    error("Unmounting and quitting")
    run(["umount", "-R", _mount_point])
    run(["rm", "-rf", _mount_point])
    sys.exit(1)


def btrfs_delete(path):
    if run(["btrfs", "subvolume", "delete", path, "--commit-after"]) != 0:
        abort(f"Failed to delete {path}")


def btrfs_delete_recursively(path):
    if not os.path.isdir(path):
        return
    result = subprocess.run(
        ["btrfs", "subvolume", "list", "-o", path],
        capture_output=True, text=True, check=False,
    )
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 9:
            subvol = os.path.join(
                os.path.dirname(path), " ".join(parts[8:]),
            )
            btrfs_delete_recursively(subvol)
    btrfs_delete(path)


def btrfs_snapshot(src, dst):
    if not os.path.isdir(src):
        abort(f"Snapshot source missing: {src}")
    btrfs_delete_recursively(dst)
    if run(["btrfs", "subvolume", "snapshot", src, dst]) != 0:
        abort(f"Failed to snapshot {src} to {dst}")


def btrfs_set_rw(path):
    if run(["btrfs", "property", "set", "-ts", path, "ro", "false"]) != 0:
        abort(f"Failed to make {path} read-write")


def btrfs_sync(path):
    run(["btrfs", "filesystem", "sync", path])


def mount_subvolumes(disk):
    if not os.path.exists(disk):
        abort(f"Device not found: {disk}")
    run(["mkdir", "-p", _mount_point])
    if run(["mount", "-t", "btrfs",
            "-o", "subvolid=5,user_subvol_rm_allowed",
            disk, _mount_point]) != 0:
        abort(f"Failed to mount {disk}")


def unmount_subvolumes():
    run(["umount", _mount_point])
    run(["rm", "-rf", _mount_point])


def check_recovery_needed(snapshot):
    return (os.path.isdir(snapshot)
            and not os.path.isfile(f"{snapshot}/.boot-ready"))


def create_sentinel(snapshot):
    with open(f"{snapshot}/.boot-ready", "w", encoding="utf-8"):
        pass


def copy_persistent_files(previous, current, filter_file):
    log("Preserving persistent paths (precomputed filter)")
    if run(["rsync", "-aHAX", "--numeric-ids", "--delete",
            f"--filter=. {filter_file}", f"{previous}/", current],
           ) != 0:
        abort("rsync failed to copy persistent files")


def reset_subvolume(name, _mount_point_arg, snapshots_name, clean_name,
                    filter_file):
    subvolume = f"{_mount_point}/{name}"
    snapshots = f"{_mount_point}/{snapshots_name}/{name}"
    clean = f"{snapshots}/{clean_name}"
    previous = f"{snapshots}/PREVIOUS"
    penultimate = f"{snapshots}/PENULTIMATE"
    current = f"{snapshots}/CURRENT"

    if check_recovery_needed(current):
        warn("Incomplete boot detected (missing .boot-ready sentinel)")
        btrfs_delete_recursively(current)

    if not os.path.isdir(clean):
        abort(f"CLEAN snapshot missing: {clean}")
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
    btrfs_sync(subvolume)


def restore_subvolume(name, snapshots_name, snapshot_label):
    subvolume = f"{_mount_point}/{name}"
    snapshots = f"{_mount_point}/{snapshots_name}/{name}"
    source = f"{snapshots}/{snapshot_label}"

    if not os.path.isdir(source):
        abort(f"Cannot restore: {source} does not exist")

    log(f"Restoring {name} from {snapshot_label}")
    btrfs_snapshot(source, subvolume)
    btrfs_sync(subvolume)
    log(f"Restored {name} from {snapshot_label}")


def snapshot_only(name, snapshots_name, clean_name):
    subvolume = f"{_mount_point}/{name}"
    snapshots = f"{_mount_point}/{snapshots_name}/{name}"
    clean = f"{snapshots}/{clean_name}"
    previous = f"{snapshots}/PREVIOUS"
    penultimate = f"{snapshots}/PENULTIMATE"

    if not os.path.isdir(clean):
        abort(f"CLEAN snapshot missing: {clean}")
    if not os.path.isdir(penultimate):
        btrfs_snapshot(clean, penultimate)
    if not os.path.isdir(previous):
        btrfs_snapshot(clean, previous)

    btrfs_snapshot(previous, penultimate)
    btrfs_snapshot(subvolume, previous)
    btrfs_sync(subvolume)
    log(f"Snapshot-only complete for {name} (no wipe)")


def process_subvolume(pair, mode, snapshots_name, clean_name, filter_files):
    parts = pair.split("=", 1)
    name = parts[0]
    mount_point = parts[1] if len(parts) > 1 else "/"

    if mode == "reset":
        log(f"Resetting {name} (mount_point={mount_point})")
        filter_file = filter_files.get(name, "")
        reset_subvolume(name, mount_point, snapshots_name, clean_name,
                        filter_file)
    elif mode == "snapshot-only":
        log(f"Snapshot-only {name}")
        snapshot_only(name, snapshots_name, clean_name)
    elif mode in ("restore-previous", "restore-penultimate"):
        label = mode.split("-", 1)[1].upper()
        log(f"Restoring {name} from {label}")
        restore_subvolume(name, snapshots_name, label)
    else:
        abort(f"Unknown mode: {mode}")


def main():
    if len(sys.argv) < 5:
        error(f"Usage: {sys.argv[0]} <device> <snapshots_name>"
              " <clean_name> <mode> [name=mount:filter ...]")
        sys.exit(1)

    device = sys.argv[1]
    snapshots_name = sys.argv[2]
    clean_name = sys.argv[3]
    mode = sys.argv[4]
    pair_args = sys.argv[5:]

    pairs = []
    filter_files = {}
    for arg in pair_args:
        if ":" in arg:
            pair, filt = arg.rsplit(":", 1)
            pairs.append(pair)
            filter_files[pair.split("=")[0]] = filt
        else:
            pairs.append(arg)

    subvolume_names = [p.split("=")[0] for p in pairs]
    log(f"Mode={mode} device={device}"
        f" subvolumes={' '.join(subvolume_names)}")
    mount_subvolumes(device)

    if mode == "disabled":
        log("Immutability disabled; skipping all operations")
        unmount_subvolumes()
        return

    if len(pairs) > 1:
        threads = []
        errors = []
        for pair in pairs:
            def worker(p=pair):
                try:
                    process_subvolume(p, mode, snapshots_name, clean_name,
                                     filter_files)
                except SystemExit:
                    errors.append(p)
            t = threading.Thread(target=worker)
            threads.append(t)
            t.start()
        for t in threads:
            t.join()
        if errors:
            abort(f"Failed subvolumes: {' '.join(errors)}")
    else:
        for pair in pairs:
            process_subvolume(pair, mode, snapshots_name, clean_name,
                              filter_files)

    unmount_subvolumes()
    log("Immutability complete")


if __name__ == "__main__":
    main()
