#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Config, Utils

def main():
    args = Utils.parse_args(["expression"])
    Utils.print(Config.eval(args.expression))

if __name__ == "__main__":
    main()
