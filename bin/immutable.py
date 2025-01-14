#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
from nixos import Immutability

def parse_args():
    parser = argparse.ArgumentParser(description="Utility to Enforce NixOS immutability")
    subparsers = parser.add_subparsers(dest="command", help="Commands")
    subparsers.add_parser("initialize", help="Create initial snapshots")
    revert_parser = subparsers.add_parser("revert", help="Revert any changed paths")
    revert_parser.add_argument("keep_paths", help="Space-delimited string of paths to keep (e.g. '/path1 /path2')", type=str, default="")
    return parser

def main():
    parser = parse_args()
    args = parser.parse_args()
    mange_mounting = False
    try:
        if not Immutability.is_btrfs_mounted():
            Immutability.mount_root()
            mange_mounting = True
        if args.command == "initialize": Immutability.create_initial_snapshots()
        elif args.command == "revert": Immutability.revert_changes(args.keep_paths)
        else: parser.print_help()
    finally:
        if mange_mounting:
            Immutability.umount_root()

if __name__ == "__main__": main()
