#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
from nixos import Utils, Config, Shell

def main():
    Utils.require_root()
    sh = Shell()
    parser = argparse.ArgumentParser()
    parser.add_argument("--rebuild-filesystem", action="store_true", help="Whether to rebuild the filesystem.")
    parser.add_argument("--reboot", action="store_true", help="Whether to reboot the system after a successful update.")
    args = parser.parse_args()

    return Config.update(rebuild_file_system=args.rebuild_filesystem, reboot=args.reboot)

if __name__ == "__main__": main()
