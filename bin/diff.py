#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os
from nixos import Utils, Snapshot, Shell, Config

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
    clean_snapshot_path = Snapshot.get_clean_snapshot_path(subvolume_name)
    output = Shell.stdout(sh.run(f"{os.path.dirname(os.path.realpath(__file__))}/diff.sh {clean_snapshot_path} {tmp_snapshot_path}", capture_output=True, check=True))
    delete_tmp_snapshot(subvolume_name)
    return output.split("\n")

def main():
    diffs = []
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot():
        try:
            diffs += diff(subvolume_name, subvolume_mount_point)
        except BaseException as e: Utils.log_error(f"Failed to create a clean snapshot for {subvolume_name}\n{e}")
    diffs = sorted(set(diffs))
    paths_to_keep = Config.eval("config.settings.disk.immutability.persist.paths").split("\n")
    Utils.print("PATHS THAT WILL BE ERASED ON BOOT")
    paths_to_ignore = []
    for change in diffs:
        if not any(change.startswith(path_to_keep) for path_to_keep in paths_to_keep):
            print(change)
        else:
            paths_to_ignore += change
    Utils.print("PATHS THAT CHANGED – BUT WILL PERSIST ON BOOT")
    for change in paths_to_ignore:
        print(change)

if __name__ == "__main__": main()
