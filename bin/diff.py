#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, argparse, fnmatch, os
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Utils, Snapshot, Shell, Config

sh = Shell(root_required=True)

def get_keep_paths():
    raw = str(Config.eval("config.settings.disk.immutability.persist.paths"))
    return raw.replace("[", "").replace("]", "").strip().split(" ")

def get_changed_files():
    changed = set()
    for name, mount in Snapshot.get_subvolumes_to_reset_on_boot():
        snapshots = Snapshot.get_snapshots_path()
        tmp = f"{snapshots}/{name}/tmp"
        clean = Snapshot.get_clean_snapshot_path(name)
        if sh.exists(tmp): sh.run(f"btrfs subvolume delete -C {tmp}")
        sh.run(f"btrfs subvolume snapshot -r {mount} {tmp}")
        transaction_id = Shell.stdout(sh.run(
            f'echo "$(sudo btrfs subvolume find-new "{clean}" 9999999)" '
            f"| cut -d' ' -f4"))
        output = Shell.stdout(sh.run(
            f'btrfs subvolume find-new "{tmp}" {transaction_id} '
            f"| sed '$d' | cut -f17- -d' ' | sort | uniq"))
        sh.run(f"btrfs subvolume delete -C {tmp}")
        for path in output.split("\n"):
            if path.strip(): changed.add(f"{mount}/{path}".replace("//", "/"))
    return changed

def top_ancestor(path, keep_paths, mount_points):
    segments = path.split("/")
    for depth in range(1, len(segments) + 1):
        ancestor = "/".join(segments[:depth]) or "/"
        if ancestor in mount_points: continue
        if any(keep.startswith(ancestor + "/") for keep in keep_paths): continue
        return ancestor
    return path

def collapse(ephemeral, keep_paths, mount_points):
    return sorted(set(
        top_ancestor(path, keep_paths, mount_points) for path in ephemeral))

def at_depth(bases, ephemeral, depth):
    if depth == 0: return list(bases)
    result = set()
    for base in bases:
        prefix = base + "/"
        for path in ephemeral:
            if not path.startswith(prefix): continue
            if depth is None: result.add(path)
            else: result.add("/".join(path.split("/")[:base.count("/") + 1 + depth]))
    return sorted(result)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--recent", action="store_true")
    parser.add_argument("--show-symlinks", action="store_true")
    parser.add_argument("--show-children", type=str, metavar="PATH")
    parser.add_argument("--depth", type=int, default=None)
    parser.add_argument("--pattern", type=str, metavar="GLOB")
    parser.add_argument("--diffignore", type=str, metavar="FILE")
    args = parser.parse_args()
    cache_path = "/tmp/etc/nixos/bin/diff/cache.json"
    ignore_path = args.diffignore or "/etc/nixos/bin/.diffignore"
    diffignore = (sh.file_read(ignore_path).split("\n")
                  if sh.exists(ignore_path) else [])
    keep_paths = get_keep_paths()
    mount_points = {mount for _, mount in Snapshot.get_subvolumes_to_reset_on_boot()}
    changed = get_changed_files()
    ephemeral = set()
    for path in changed:
        if any(path == keep or path.startswith(keep + "/")
               for keep in keep_paths): continue
        if any(fnmatch.fnmatch(path, pattern)
               for pattern in diffignore): continue
        ephemeral.add(path)
    top_changes = collapse(ephemeral, keep_paths, mount_points)
    previous = sh.json_read(cache_path)
    sh.json_overwrite(cache_path, {path: True for path in top_changes})
    output = list(top_changes)
    if args.recent:
        output = [path for path in output if path not in previous]
    if not args.show_symlinks:
        output = [path for path in output if not os.path.islink(path)]
    if args.pattern:
        output = [path for path in output
                  if fnmatch.fnmatch(path.lower(), args.pattern.lower())]
    if args.show_children:
        target = args.show_children
        covers = any(path == target or target.startswith(path + "/")
                     or path.startswith(target + "/") for path in output)
        output = [target] if covers else []
        depth = args.depth
    else:
        depth = args.depth or 0
    output = at_depth(output, ephemeral, depth)
    if output:
        Utils.print_error("\nCHANGES TO DELETE:")
        for path in output: Utils.print_error(path)

if __name__ == "__main__":
    main()
