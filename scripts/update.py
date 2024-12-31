#!/usr/bin/env python3
from scripts.utils import Utils, Config

def main():
    Utils.require_root()
    return Config.update()

if __name__ == "__main__": main()
