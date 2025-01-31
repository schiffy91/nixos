#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os, argparse, hashlib, fnmatch
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
        if file_path.startswith(subvolume_mount_point): previous_file_path = f"{clean_snapshot_path}/{file_path.replace(clean_snapshot_path, '')}".replace("//", "/")
    if previous_file_path == "": Utils.abort(f"Couldn't diff {file_path}")
    current_file_path = file_path
    try: return Shell.stdout(sh.run(f"diff -u {previous_file_path} {current_file_path}", capture_output=True))
    except BaseException: pass
    try: return sh.file_read(current_file_path)
    except BaseException: return "N/A"

def diff_files(file_paths):
    diffs = {}
    for file in file_paths: diffs[file] = diff_file(file)
    return diffs

def get_diffs(previous_run, paths_to_keep, paths_to_hide):
    diffs = set()
    for subvolume_name, subvolume_mount_point in Snapshot.get_subvolumes_to_reset_on_boot(): diffs.update(diff_subvolume(subvolume_name, subvolume_mount_point))
    diffs = sorted(diffs)
    diff_paths_to_delete = set()
    diff_paths_to_ignore = set()
    diff_paths_to_hide = set()
    diff_paths_hashed = {}
    diff_paths_since_last_run_hashed = {}
    for diff in diffs:
        if any(diff.startswith(path_to_keep) for path_to_keep in paths_to_keep):
            diff_paths_to_ignore.add(diff)
        else:
            diff_paths_to_delete.add(diff)
            diff_hash = "N/A"
            if not os.path.isdir(diff) and not (os.path.islink(diff) and not os.path.exists(diff)): diff_hash = sha256sum(diff)
            diff_paths_hashed[diff] = diff_hash
            if paths_to_keep is not None and any(fnmatch.fnmatch(diff, pattern) for pattern in paths_to_hide): diff_paths_to_hide.add(diff)
            if previous_run is None: continue
            if diff_hash != previous_run.get(diff, ""): diff_paths_since_last_run_hashed[diff] = diff_hash
    return (sorted(diff_paths_to_delete), sorted(diff_paths_to_ignore), diff_paths_hashed, diff_paths_since_last_run_hashed, sorted(diff_paths_to_hide))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--since-last-run", action="store_true", help="Only list changes since the last run of this program.")
    parser.add_argument("--show-changes-to-ignore", action="store_true", help="List any changes that will be ignored because they match paths to keep (usually located in /etx/nixos/modules/settings.nix).")
    parser.add_argument("--show-paths-to-keep", action="store_true", help="List the paths to keep (usually located in /etx/nixos/modules/settings.nix)")
    parser.add_argument("--delta", nargs="*", metavar="FILE", default=[], help="Show the delta of one or many files")
    parser.add_argument("--deltas", action="store_true", help="Show the delta of every changed file. Aware of --since-last-run supplied and --paths-to-hide.")
    parser.add_argument("--paths-to-hide", type=str, metavar="FILE", help="A file that specifies new-line separated paths (supporting * wildcards) to hide from this script's output. They'll still be deleted upon boot if they're not matched by paths to keep (usually located in /etc/nixos/modules/settings.nix).")
    args = parser.parse_args()

    diff_json_file_path = "/tmp/etc/nixos/bin/diff/diff.json"
    previous_run = sh.json_read(diff_json_file_path) if args.since_last_run else None
    paths_to_keep = get_paths_to_keep()
    paths_to_hide = sh.file_read(args.paths_to_hide).split("\n") if args.paths_to_hide else []

    diff_paths_to_delete, diff_paths_to_ignore, diff_paths_hashed, diff_paths_since_last_run_hashed, diff_paths_to_hide = get_diffs(previous_run, paths_to_keep, paths_to_hide)
    diffs_to_print = sorted(set(diff_paths_since_last_run_hashed.keys()).difference(diff_paths_to_hide)) if args.since_last_run else sorted(set(diff_paths_to_delete).difference(diff_paths_to_hide))
    delta = diff_files(args.delta)
    deltas = diff_files(diffs_to_print) if args.deltas else {}

    if len(diff_paths_to_delete) != 0:
        sh.json_overwrite(diff_json_file_path, diff_paths_hashed)
        if len(diffs_to_print) != 0:
            Utils.print_warning("\nCHANGES TO DELETE:")
            Utils.print_warning("\n".join(sorted(diffs_to_print)))
    else: sh.rm(diff_json_file_path)

    if args.show_changes_to_ignore:
        Utils.print("\nCHANGES TO IGNORE:")
        Utils.print("\n".join(sorted(diff_paths_to_ignore)))

    if args.show_paths_to_keep:
        Utils.print("\nPATHS TO KEEP:")
        for path in get_paths_to_keep(): Utils.print(path)

    if len(delta) != 0:
        Utils.print("\nFILE DIFFS:")
        for file, diff in delta.items(): print(f"File: {file}\n{diff}")

    if args.deltas and len(deltas) != 0:
        Utils.print("\nFILES THAT CHANGED DIFFS:")
        for file, diff in deltas.items(): print(f"File: {file}\n{diff}")

if __name__ == "__main__": main()
