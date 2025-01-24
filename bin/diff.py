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
    transaction_id = Shell.stdout(sh.run(f"echo \"$(sudo btrfs subvolume find-new \"{clean_snapshot_path}\" 9999999)\" | cut -d' ' -f4", capture_output=True, check=True))
    output = Shell.stdout(sh.run(f"btrfs subvolume find-new \"{tmp_snapshot_path}\" {transaction_id} | sed '$d' | cut -f17- -d' ' | sort | uniq", capture_output=True, check=True))
    delete_tmp_snapshot(subvolume_name)
    output = [ os.path.normpath(f"{subvolume_mount_point}/{path}").replace("//", "/") for path in output.split("\n") ]
    return set(output)
def main():
    paths_to_keep = Config.eval("config.settings.disk.immutability.persist.paths").replace("[", "").replace("]", "").strip().split(" ")
    diffs = set()
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot():
        try: diffs.update(diff(subvolume_name, subvolume_mount_point))
        except BaseException as e: Utils.log_error(f"Failed to create a clean snapshot for {subvolume_name}\n{e}")
    diffs = sorted(diffs)
    changes_to_delete = set()
    changes_to_ignore = set()
    for change in diffs:
        if not any(change.startswith(path_to_keep) for path_to_keep in paths_to_keep): changes_to_delete.add(change)
        else: changes_to_ignore.add(change)
    changes_to_delete = sorted(changes_to_delete)
    changes_to_ignore = sorted(changes_to_ignore)
    Utils.print_warning("\nCHANGES TO DELETE:")
    for change_to_delete in changes_to_delete: Utils.print_warning(change_to_delete)
    Utils.print("\nPATHS TO KEEP:")
    for path_to_keep in paths_to_keep: Utils.print(path_to_keep)
    Utils.print("\nCHANGES TO IGNORE:")
    for change_to_ignore in changes_to_ignore: print(change_to_ignore)

if __name__ == "__main__": main()
