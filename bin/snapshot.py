#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
from nixos import Snapshot
def main(): Snapshot.manage_snapshots()
if __name__ == "__main__": main()
