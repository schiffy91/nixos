#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os
from nixos import Utils, Snapshot, Shell

sh = Shell(root_required=True)

def get_tmp_snapshot_path(subvolume_name): return f"{Snapshot.get_snapshots_path()}/{subvolume_name}/tmp"
def delete_tmp_snapshot(subvolume_name):
    tmp_snapshot_path = get_tmp_snapshot_path(subvolume_name)
    if sh.exists(tmp_snapshot_path): sh.run(f"btrfs subvolume delete -C {tmp_snapshot_path}")
def create_tmp_snapshot(subvolume_name, subvolume_mount_point):
    tmp_snapshot_path = get_tmp_snapshot_path(subvolume_name)
    delete_tmp_snapshot(subvolume_name)
    sh.run(f"btrfs subvolume snapshot -r {subvolume_mount_point} {tmp_snapshot_path}")
    return tmp_snapshot_path
def diff(subvolume_name, subvolume_mount_point):
    tmp_snapshot_path = create_tmp_snapshot(subvolume_name, subvolume_mount_point)
    diff_name = "tmp_diff"
    clean_snapshot_path = Snapshot.get_clean_snapshot_path(subvolume_name)
    #sh.run(f"{os.path.dirname(os.path.realpath(__file__))}/btrfs_diff.sh {clean_snapshot_path} {tmp_snapshot_path}", capture_output=False, check=False)
    sh.run(f"btrfs send --no-data -p {clean_snapshot_path} {tmp_snapshot_path} > {diff_name}", capture_output=False, check=False)
    sh.run(f"{os.path.dirname(os.path.realpath(__file__))}/btrfs-snapshots-diff.py -f {diff_name}", capture_output=False, check=False)
    #sh.run(f"{os.path.dirname(os.path.realpath(__file__))}/btrfs-snapshots-diff.py -p {clean_snapshot_path} -c {tmp_snapshot_path}", capture_output=False, check=False)
    delete_tmp_snapshot(subvolume_name)

def main():
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot():
        try: diff(subvolume_name, subvolume_mount_point)
        except BaseException as e: Utils.log_error(f"Failed to create a clean snapshot for {subvolume_name}\n{e}")

if __name__ == "__main__": main()
