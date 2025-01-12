#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, json
from nixos import Utils, Config

def main():
    if len(sys.argv[1:]) != 1: return Utils.abort("Usage: eval.py attribute\nExample: eval.py config.settings.disk.device")
    try: Utils.print(Config.eval(sys.argv[1:][0]))
    except BaseException as exception:
        Utils.log_error(f"Caught exception: {exception}.")
        raise

if __name__ == "__main__": main()
