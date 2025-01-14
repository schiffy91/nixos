#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
from nixos import Immutability, Shell
from install import Installer

def parse_args():
    parser = argparse.ArgumentParser(description="Utility to Enforce NixOS immutability")
    subparsers = parser.add_subparsers(dest="command", help="Commands")
    subparsers.add_parser("initialize", help="Create initial snapshots")
    revert_parser = subparsers.add_parser("revert", help="Revert any changed paths")
    revert_parser.add_argument("paths_to_keep", help="Space-delimited string of paths to keep (e.g. '/path1 /path2')", type=str, default="")
    return parser
def initialize(): Immutability.create_initial_snapshots()
def revert(paths_to_keep): Immutability.revert_changes(paths_to_keep)
def mount():
    if not Immutability.is_btrfs_mounted():
        Immutability.mount_root()
        return True
    return False
def unmount(): Immutability.umount_root()

def main():
    parser = parse_args()
    args = parser.parse_args()
    sh = Shell()
    with sh.chroot(Installer.get_mount_point()):
        mange_mounting = mount()
        if args.command == "initialize": initialize()
        elif args.command == "revert": revert(args.paths_to_keep)
        else: parser.print_help()
        if mange_mounting: unmount()
            

if __name__ == "__main__": main()
