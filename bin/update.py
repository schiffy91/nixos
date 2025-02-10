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
    parser.add_argument("--clean", action="store_true", help="Runs 'nix-collect-garbage -d', 'rm -rf /root/.cache/', and 'nix-store --verify --repair'")
    parser.add_argument("--upgrade", action="store_true", help="Runs 'nix flake update'")
    args = parser.parse_args()

    if args.clean:
        sh.run("nix-collect-garbage -d", capture_output=False)
        sh.run("rm -rf /root/.cache")
        sh.run("nix-store --verify --repair", capture_output=False)
    if args.upgrade: sh.run(f"nix flake update --flake {Config.get_nixos_path()}", capture_output=False)

    return Config.update(rebuild_file_system=args.rebuild_filesystem, reboot=args.reboot)

if __name__ == "__main__": main()
