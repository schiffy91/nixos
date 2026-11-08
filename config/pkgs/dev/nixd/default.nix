{ system ? builtins.currentSystem }:
let
  lock = builtins.fromJSON (builtins.readFile ../../../../flake.lock);
  locked = lock.nodes.nixpkgs.locked;
  source = builtins.fetchTree { inherit (locked) type owner repo rev narHash; };
  pkgs = import source { inherit system; };
in import ./package.nix { inherit pkgs; }
