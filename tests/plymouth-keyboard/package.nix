let
  locked = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree {
    inherit (locked) type owner repo rev narHash;
  }) { system = builtins.currentSystem; };
in pkgs.callPackage ../../nix/system/nixos/boot/plymouth/package.nix { }
