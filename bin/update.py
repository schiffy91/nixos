#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Config, Shell, Utils

sh = Shell(root_required=True)

def main():
    args = Utils.parse_args(["--rebuild-filesystem", "--reboot", "--clean", "--upgrade"])
    delete_cache = args.clean or args.upgrade
    return Config.update(
        rebuild_file_system=args.rebuild_filesystem,
        reboot=args.reboot,
        delete_cache=delete_cache,
        upgrade=args.upgrade)

if __name__ == "__main__":
    main()