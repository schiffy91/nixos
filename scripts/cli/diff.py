#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
import difflib
import fnmatch
import hashlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from core import Utils, Snapshot, Shell, Config

sh = Shell(root_required=True)


def get_tmp_snapshot_path(subvolume_name):
    return f"{Snapshot.get_snapshots_path()}/{subvolume_name}/tmp"


def get_paths_to_keep():
    raw = str(Config.eval("config.settings.disk.immutability.persist.paths"))
    return raw.replace("[", "").replace("]", "").strip().split(" ")


def delete_tmp_snapshot(subvolume_name):
    tmp = get_tmp_snapshot_path(subvolume_name)
    if sh.exists(tmp):
        sh.run(f"btrfs subvolume delete -C {tmp}")


def create_tmp_snapshot(subvolume_name, subvolume_mount_point):
    tmp = get_tmp_snapshot_path(subvolume_name)
    delete_tmp_snapshot(subvolume_name)
    sh.run(f"btrfs subvolume snapshot -r {subvolume_mount_point} {tmp}")
    return tmp


def sha256sum(file_path):
    file_hash = "N/A"
    if not sh.exists(file_path) or sh.is_dir(file_path) or sh.is_symlink(file_path):
        return file_hash
    try:
        contents = sh.file_read(file_path)
        return hashlib.sha256(contents.encode()).hexdigest()
    except (OSError, PermissionError):
        return file_hash


def diff_subvolume(subvolume_name, subvolume_mount_point):
    tmp = create_tmp_snapshot(subvolume_name, subvolume_mount_point)
    clean = Snapshot.get_clean_snapshot_path(subvolume_name)
    txid = Shell.stdout(sh.run(
        f'echo "$(sudo btrfs subvolume find-new "{clean}" 9999999)" '
        f"| cut -d' ' -f4",
        capture_output=True, check=True,
    ))
    output = Shell.stdout(sh.run(
        f'btrfs subvolume find-new "{tmp}" {txid} '
        f"| sed '$d' | cut -f17- -d' ' | sort | uniq",
        capture_output=True, check=True,
    ))
    delete_tmp_snapshot(subvolume_name)
    paths = [
        f"{subvolume_mount_point}/{p}".replace("//", "/")
        for p in output.split("\n")
    ]
    return set(paths)


_mount_cache = {}


def get_mount_cache():
    if _mount_cache:
        return _mount_cache
    for name, mount in Snapshot.get_subvolumes_to_reset_on_boot():
        _mount_cache[mount] = Snapshot.get_clean_snapshot_path(name)
    return _mount_cache


def diff_file(file_path):
    if not sh.exists(file_path):
        return "N/A (DOES NOT EXIST)"
    if sh.is_dir(file_path):
        return "N/A (DIRECTORY)"
    if sh.is_symlink(file_path):
        return "N/A (LINK)"
    previous_file_path = ""
    match = ""
    for mount, clean in get_mount_cache().items():
        if file_path.startswith(mount) and len(match) < len(mount):
            previous_file_path = (
                f"{clean}/{file_path.replace(clean, '')}"
            ).replace("//", "/")
            match = mount
    if not previous_file_path:
        try:
            return sh.file_read(file_path)
        except (OSError, PermissionError):
            return "N/A (NEW BINARY FILE)"
    try:
        current = sh.file_read(file_path).strip().splitlines()
        previous = sh.file_read(previous_file_path).strip().splitlines()
        return "\n".join(difflib.unified_diff(
            previous, current,
            fromfile=previous_file_path, tofile=file_path, lineterm="",
        ))
    except (OSError, PermissionError, UnicodeDecodeError):
        return "N/A (EXISTING BINARY FILE)"


def diff_files(file_paths):
    diffs = {}
    for i, f in enumerate(file_paths):
        Utils.print_inline(
            f"Progress: {(i + 1) / float(len(file_paths)) * 100:.2f}%"
        )
        diffs[f] = diff_file(f)
    return diffs


def get_diffs(previous_run, diffignore):
    paths_to_keep = get_paths_to_keep()
    diffs = set()
    for name, mount in Snapshot.get_subvolumes_to_reset_on_boot():
        diffs.update(diff_subvolume(name, mount))
    diffs = sorted(diffs)
    to_delete = set()
    to_keep = set()
    to_diffignore = set()
    hashed = {}
    recent_hashed = {}
    for i, d in enumerate(diffs):
        Utils.print_inline(
            f"Progress: {(i + 1) / float(len(diffs)) * 100:.2f}%"
        )
        if any(d.startswith(p) for p in paths_to_keep):
            to_keep.add(d)
        else:
            to_delete.add(d)
            h = sha256sum(d)
            hashed[d] = h
            if any(fnmatch.fnmatch(d, pat) for pat in diffignore):
                to_diffignore.add(d)
            if previous_run is None:
                continue
            if h != previous_run.get(d, ""):
                recent_hashed[d] = h
    return (
        sorted(to_delete), sorted(to_keep),
        hashed, recent_hashed, sorted(to_diffignore),
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--recent", action="store_true")
    parser.add_argument("--show-changes-to-keep", action="store_true")
    parser.add_argument("--show-paths-to-keep", action="store_true")
    parser.add_argument("--deltas", nargs="*", metavar="FILE", default=None)
    parser.add_argument("--diffignore", type=str, metavar="FILE")
    parser.add_argument("--dirname", action="store_true")
    args = parser.parse_args()

    diff_json = "/tmp/etc/nixos/scripts/cli/diff/diff.json"
    ignore_path = (
        args.diffignore
        if args.diffignore
        else "/etc/nixos/scripts/cli/.diffignore"
    )
    previous_run = sh.json_read(diff_json) if args.recent else None
    diffignore = (
        sh.file_read(ignore_path).split("\n")
        if sh.exists(ignore_path) else []
    )

    to_delete, to_keep, hashed, recent_hashed, to_diffignore = get_diffs(
        previous_run, diffignore
    )
    if args.recent:
        to_print = sorted(
            set(recent_hashed.keys()).difference(to_diffignore)
        )
    else:
        to_print = sorted(set(to_delete).difference(to_diffignore))
    deltas = {}
    if args.deltas is not None:
        if args.deltas:
            deltas = diff_files(args.deltas)
        else:
            deltas = diff_files(to_print)
    if args.dirname:
        to_print = sorted(set(sh.dirname(d) for d in to_print))

    if to_delete:
        sh.json_overwrite(diff_json, hashed)
        if to_print:
            Utils.print_warning("\nCHANGES TO DELETE:")
            Utils.print_warning("\n".join(sorted(to_print)))
    else:
        sh.rm(diff_json)

    if args.show_changes_to_keep:
        Utils.print("\nCHANGES TO IGNORE:")
        Utils.print("\n".join(sorted(to_keep)))

    if args.show_paths_to_keep:
        Utils.print("\nPATHS TO KEEP:")
        for p in get_paths_to_keep():
            Utils.print(p)

    if deltas:
        Utils.print("\nDELTAS:")
        failures = set()
        for path, d in deltas.items():
            output = f"\n{path}\n{d}"
            if d.startswith("N/A") or not d.strip():
                failures.add(output)
            else:
                print(output)
        Utils.print("\nN/A:")
        for output in sorted(failures):
            print(output)


if __name__ == "__main__":
    main()
