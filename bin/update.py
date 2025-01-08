#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
from utils import Utils, Config

def main():
    Utils.require_root()
    return Config.update()

if __name__ == "__main__": main()
