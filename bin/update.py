#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from nixos import Utils, Config, Shell

def main():
    Utils.require_root()
    sh = Shell()
    sh.run(f"cd {Config.get_nixos_path()} && git pull")
    rebuild_filesystem = "--rebuild_filesystem" in sys.argv
    return Config.update(rebuild_file_system=rebuild_filesystem)

if __name__ == "__main__": main()
