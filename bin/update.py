#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
from nixos import Utils, Config, Shell

def main():
    Utils.require_root()
    sh = Shell()
    sh.run(f"git -C {Config.get_nixos_path()} pull", capture_output=False, check=False)
    parser = argparse.ArgumentParser()
    parser.add_argument("--rebuild-filesystem", action="store_true", help="Whether to rebuild the filesystem.")
    args = parser.parse_args()

    return Config.update(rebuild_file_system=args.rebuild_filesystem)

if __name__ == "__main__": main()
