#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from nixos import Utils, Config

def main(): 
    args = sys.argv[1:]
    if len(args) != 1: return Utils.abort("Usage: eval.py attribute\nExample: eval.py config.settings.disk.device")
    try:
        Utils.print(Config.eval(sys.argv[1]))
    except BaseException as exception:
        Utils.log_error(f"Caught exception: {exception}.")
        raise

if __name__ == "__main__": main()
