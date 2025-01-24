#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os, argparse, hashlib
from nixos import Utils, Snapshot, Shell, Config

sh = Shell(root_required=True)

def get_tmp_snapshot_path(subvolume_name): return f"{Snapshot.get_snapshots_path()}/{subvolume_name}/tmp"

def get_paths_to_keep(): return Config.eval("config.settings.disk.immutability.persist.paths").replace("[", "").replace("]", "").strip().split(" ")

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

def get_diffs():
    diffs = set()
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot():
        try: diffs.update(diff(subvolume_name, subvolume_mount_point))
        except BaseException as e: Utils.log_error(f"Failed to create a clean snapshot for {subvolume_name}\n{e}")
    return sorted(diffs)

def get_changes():
    diffs = get_diffs()
    changes_to_delete = set()
    changes_to_ignore = set()
    paths_to_keep = get_paths_to_keep()
    for change in diffs:
        if not any(change.startswith(path_to_keep) for path_to_keep in paths_to_keep): changes_to_delete.add(change)
        else: changes_to_ignore.add(change)
    return (sorted(changes_to_delete), sorted(changes_to_ignore))

def sha256sum(filename):
    with open(filename, 'rb', buffering=0) as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def print_changes(changes, diff_json=None):
    hashes = {}
    for change in changes:
        if os.path.isdir(change): continue
        change_hash = sha256sum(change)
        if diff_json is not None and change_hash != diff_json.get(change, ""):
            Utils.print(change)
        hashes[change] = change_hash
    return hashes

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--since-last-run", help="Only list changes since the last run of this program ")
    parser.add_argument("--show-changes-to-ignore", help="List changes that will be ignored because they match paths to keep")
    parser.add_argument("--show-paths-to-keep", help="List paths to keep (usually located in /etx/nixos/modules/settings.nix)")
    args = parser.parse_args()

    diff_json_file_path = "/tmp/etc/nixos/bin/diff/diff.json"
    diff_json = sh.json_read(diff_json_file_path) if args.since_last_run else None

    changes_to_delete, changes_to_ignore = get_changes()
    if len(changes_to_delete) != 0:
        Utils.print_warning("\nCHANGES TO DELETE:")
        hashes = print_changes(changes_to_delete, diff_json)
        sh.json_overwrite(diff_json_file_path, hashes)
    else: sh.rm(diff_json_file_path)

    if args.show_changes_to_ignore:
        Utils.print("\nCHANGES TO IGNORE:")
        print_changes(changes_to_ignore)

    if args.show_paths_to_keep:
        Utils.print("\nPATHS TO KEEP:")
        for path in get_paths_to_keep(): Utils.print(path)

if __name__ == "__main__": main()
