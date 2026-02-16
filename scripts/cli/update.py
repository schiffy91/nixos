#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from core import Config, Shell


def main():
    Shell(root_required=True)
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--rebuild-filesystem", action="store_true",
    )
    parser.add_argument("--reboot", action="store_true")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--upgrade", action="store_true")
    args = parser.parse_args()
    delete_cache = args.clean or args.upgrade
    return Config.update(
        rebuild_file_system=args.rebuild_filesystem,
        reboot=args.reboot,
        delete_cache=delete_cache,
        upgrade=args.upgrade,
    )


if __name__ == "__main__":
    main()
