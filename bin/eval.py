#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, argparse
from nixos import Utils, Config

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("expression", help="Full expression to evaluate (e.g. config.settings.disk.device).")
    args = parser.parse_args()
    try: Utils.print(Config.eval(args.expression))
    except BaseException as exception:
        Utils.log_error(f"Caught exception: {exception}.")
        raise

if __name__ == "__main__": main()
