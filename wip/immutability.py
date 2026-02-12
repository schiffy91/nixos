#!/usr/bin/env python3
import argparse, os, subprocess, sys

def abort(message=None, path_to_unmount=None):
    try:
        if path_to_unmount: disk_unmount(path_to_unmount)
        if message: print(message)
    except BaseException: pass
    sys.exit(1)
def run(cmd, check=True): return subprocess.run(cmd, shell=True, check=check)
def set_up(disk, mount_point, snapshots_subvolume_name, subvolume_names):
    if not device_exists(disk): abort(f"{disk} does not exist")
    create_directory(mount_point)
    disk_mount_root(disk, mount_point)
    for subvolume_name in subvolume_names + [snapshots_subvolume_name]:
        create_directory(f"{mount_point}/{subvolume_name}")
        disk_mount_subvolume(disk, subvolume_name, f"{mount_point}/{subvolume_name}")
    return
def tear_down(mount_point):
    if path_exists(mount_point): disk_unmount(mount_point)
def device_exists(device): return run(f"-b {device}", check=False).returncode
def path_exists(*args): return run(" && ".join(f"[ -e '{arg}' ]" for arg in args) + " && true", check=False).returncode
def dirname(path): return os.path.dirname(path)
def create_directory(path): return run(f"mkdir -p {path}", check=False).returncode
def disk_mount_root(disk, mount_point): return run (f"mount -t btrfs -o subvolid=5,user_subvol_rm_allowed '{disk}' '{mount_point}").returncode
def disk_mount_subvolume(disk, subvolume_name, mount_point): return run (f"trace mount -t btrfs -o subvol='{subvolume_name}',user_subvol_rm_allowed '{disk}' '{mount_point}/{subvolume_name}'").returncode
def disk_unmount(mount_point): return run (f"umount -R '{mount_point}' && rm -rf '{mount_point}'").returncode
def btrfs_copy(src, dst): return "TODO"
def btrfs_rw(path): run(f"btrfs property set -ts '{path}' ro false")
def btrfs_delete(subvolume_path):
    run(f"btrfs subvolume delete -R '{subvolume_path}' --commit-after")
    btrfs_sync(dirname(subvolume_path))
def btrfs_sync(path): return run(f"btrfs filesystem sync '{path}'").returncode
def rsync_preserve(old_snapshot, paths_file, new_snapshot): run(f"rsync -a --files-from='{paths_file}' '{old_snapshot}/' '{new_snapshot}/'")
def reset_subvolume(subvol_name, args):
    subvol_path = os.path.join(args.mount_point, subvol_name)
    snaps_dir   = os.path.join(args.mount_point, args.snapshots_subvol, subvol_name)
    clean_snap  = os.path.join(snaps_dir, args.clean_label)
    prev_snap   = os.path.join(snaps_dir, args.prev_label)
    penult_snap = os.path.join(snaps_dir, args.penult_label)
    curr_snap   = os.path.join(snaps_dir, args.curr_label)
    if not os.path.isdir(clean_snap): raise SystemExit(f"Missing clean snapshot: {clean_snap}")
    if not os.path.isdir(penult_snap): btrfs_copy(clean_snap, penult_snap)
    if not os.path.isdir(prev_snap): btrfs_copy(clean_snap, prev_snap)
    if os.path.isdir(penult_snap): btrfs_delete(penult_snap)
    btrfs_copy(prev_snap, penult_snap)
    if os.path.isdir(prev_snap): btrfs_delete(prev_snap)
    btrfs_copy(subvol_path, prev_snap)
    if os.path.isdir(curr_snap): btrfs_delete(curr_snap)
    btrfs_copy(clean_snap, curr_snap)
    btrfs_rw(curr_snap)
    rsync_preserve(prev_snap, args.paths_to_keep, curr_snap)
    if os.path.isdir(subvol_path): btrfs_delete(subvol_path)
    btrfs_copy(curr_snap, subvol_path)

def main():
    parser = argparse.ArgumentParser(description="Replicate Bash snapshot rotation in Python, including btrfs subvolume deletion.")
    parser.add_argument("mount_point",              help="Base btrfs mount point")
    parser.add_argument("snapshots_subvol",         help="Subvolume directory for snapshots (under the mount point)")
    parser.add_argument("clean_snapshot_name",      help="Label for the clean snapshot")
    parser.add_argument("previous_snapshot_name",   help="Label for the previous snapshot")
    parser.add_argument("penultimate_snapshot_name",help="Label for the penultimate snapshot")
    parser.add_argument("current_snapshot_name",    help="Label for the current snapshot")
    parser.add_argument("paths_to_keep",            help="File listing paths to preserve via rsync")
    parser.add_argument("pairs", nargs="+",         help="One or more 'subvolume=live_mount' pairs (e.g. appvol=/appmnt)")
    args = parser.parse_args()
    for pair in args.pairs:
        subvol_name, _ = pair.split("=", 1)
        reset_subvolume(subvol_name, args)
    btrfs_unmount(args.mount_point)

if __name__ == "__main__": main()
