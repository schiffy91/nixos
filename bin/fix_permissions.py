#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Config, Shell

def main():
    sh = Shell(root_required=True)
    username = os.environ.get("SUDO_USER") or sh.whoami()
    Config.secure(username)

if __name__ == "__main__":
    main()
