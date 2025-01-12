#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from nixos import Utils

def enable_immutability(): return

def disable_immutability(): return

def main(): Utils.toggle(sys.argv, on_enable=enable_immutability, on_disable=disable_immutability, on_exception=disable_immutability)

if __name__ == "__main__": main()
