#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os, argparse, hashlib, difflib
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

def sha256sum(filename):
    with open(filename, "rb", buffering=0) as f:
        return hashlib.file_digest(f, "sha256").hexdigest()

def diff_subvolume(subvolume_name, subvolume_mount_point):
    tmp_snapshot_path = create_tmp_snapshot(subvolume_name, subvolume_mount_point)
    clean_snapshot_path = Snapshot.get_clean_snapshot_path(subvolume_name)
    transaction_id = Shell.stdout(sh.run(f"echo \"$(sudo btrfs subvolume find-new \"{clean_snapshot_path}\" 9999999)\" | cut -d' ' -f4", capture_output=True, check=True))
    output = Shell.stdout(sh.run(f"btrfs subvolume find-new \"{tmp_snapshot_path}\" {transaction_id} | sed '$d' | cut -f17- -d' ' | sort | uniq", capture_output=True, check=True))
    delete_tmp_snapshot(subvolume_name)
    output = [ os.path.normpath(f"{subvolume_mount_point}/{path}").replace("//", "/") for path in output.split("\n") ]
    return set(output)

def diff_file(file_path):
    previous_file_path = ""
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot(): 
        clean_snapshot_path = Snapshot.get_clean_snapshot_path(subvolume_name)
        if file_path.startswith(subvolume_mount_point): previous_file_path = f"{clean_snapshot_path}/{file_path.replace(clean_snapshot_path, '')}"
    if previous_file_path == "": Utils.abort(f"Couldn't diff {file_path}")
    current_file_path = file_path
    previous_file = sh.file_read(previous_file_path)
    current_file = sh.file_read(current_file_path)
    delta = ""
    for line in difflib.unified_diff(previous_file, current_file, fromfile=previous_file_path, tofile=current_file_path, lineterm=''): delta += line
    return delta

def diff_files(file_paths):
    deltas = []
    for file in file_paths: deltas += diff_file(file)
    return deltas

def get_diffs(previous_run=None):
    diffs = set()
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot(): diffs.update(diff_subvolume(subvolume_name, subvolume_mount_point))
    diffs = sorted(diffs)
    diffs_to_delete = set()
    diffs_to_ignore = set()
    diffs_hashed = {}
    diffs_since_last_run_hashed = {}
    paths_to_keep = get_paths_to_keep()
    for diff in diffs:
        if any(diff.startswith(path_to_keep) for path_to_keep in paths_to_keep):
            diffs_to_ignore.add(diff)
        else:
            diffs_to_delete.add(diff)
            diff_hash = "N/A"
            if not os.path.isdir(diff) and not (os.path.islink(diff) and not os.path.exists(diff)): diff_hash = sha256sum(diff)
            diffs_hashed[diff] = diff_hash
            if previous_run is None: continue
            if diff_hash != previous_run.get(diff, ""): diffs_since_last_run_hashed[diff] = diff_hash
    return (sorted(diffs_to_delete), sorted(diffs_to_ignore), diffs_hashed, diffs_since_last_run_hashed)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--since-last-run", action="store_true", help="Only list changes since the last run of this program ")
    parser.add_argument("--show-changes-to-ignore", action="store_true", help="List changes that will be ignored because they match paths to keep")
    parser.add_argument("--show-paths-to-keep", action="store_true", help="List paths to keep (usually located in /etx/nixos/modules/settings.nix)")
    parser.add_argument("--files", nargs="*", default=[], help="Directory to search")
    args = parser.parse_args()

    diff_json_file_path = "/tmp/etc/nixos/bin/diff/diff.json"
    previous_run = sh.json_read(diff_json_file_path) if args.since_last_run else None

    diffs_to_delete, diffs_to_ignore, diffs_hashed, diffs_since_last_run_hashed  = get_diffs(previous_run)

    if len(diffs_to_delete) != 0:
        sh.json_overwrite(diff_json_file_path, diffs_hashed)
        Utils.print_warning("\nCHANGES TO DELETE:")
        diffs_to_print = diffs_since_last_run_hashed.keys() if args.since_last_run else diffs_to_delete
        Utils.print_warning("\n".join(sorted(diffs_to_print)))
    else: sh.rm(diff_json_file_path)

    if args.show_changes_to_ignore:
        Utils.print("\nCHANGES TO IGNORE:")
        Utils.print("\n".join(sorted(diffs_to_ignore)))

    if args.show_paths_to_keep:
        Utils.print("\nPATHS TO KEEP:")
        for path in get_paths_to_keep(): Utils.print(path)

    deltas = diff_files(args.files)
    if len(deltas) != 0:
        Utils.print("\nFILE DIFFS")
        for delta in deltas: print(delta)

if __name__ == "__main__": main()
