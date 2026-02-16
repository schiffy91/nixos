#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse

from core import Utils, Config


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("expression")
    args = parser.parse_args()
    try:
        Utils.print(Config.eval(args.expression))
    except BaseException as exception:
        Utils.log_error(f"Caught exception: {exception}.")
        raise


if __name__ == "__main__":
    main()
