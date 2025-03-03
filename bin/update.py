#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse
from nixos import Config, Shell

def main():
    sh = Shell(root_required=True)
    parser = argparse.ArgumentParser()
    parser.add_argument("--rebuild-filesystem", action="store_true", help="Whether to rebuild the filesystem.")
    parser.add_argument("--reboot", action="store_true", help="Whether to reboot the system after a successful update.")
    parser.add_argument("--clean", action="store_true", help="Runs 'nix-collect-garbage -d', 'rm -rf /root/.cache/', and 'nix-store --verify --repair'")
    parser.add_argument("--upgrade", action="store_true", help="Runs 'nix flake update'")
    args = parser.parse_args()
    
    delete_cache = args.clean or args.upgrade
    if delete_cache:
        sh.run("nix-collect-garbage -d", capture_output=False)
        sh.rm("/root/.cache")
        sh.run("nix-store --verify --repair", capture_output=False)

    if args.upgrade:
        sh.rm("/root/.cache")
        sh.run(f"nix flake update --flake {Config.get_nixos_path()}", capture_output=False)

    return Config.update(rebuild_file_system=args.rebuild_filesystem, reboot=args.reboot)

if __name__ == "__main__": main()
