#!/usr/bin/env python3
from nixos_utils import *

def main():
    Utils.require_root()
    return NixOSConfig.update()

if __name__ == "__main__": main()
