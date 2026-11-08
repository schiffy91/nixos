{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes;
  pkgs = import (builtins.fetchTree { inherit (lock.nixpkgs.locked) type owner repo rev narHash; }) { inherit system; };
  inputs.btrc = builtins.getFlake "github:${lock.btrc.locked.owner}/${lock.btrc.locked.repo}/${lock.btrc.locked.rev}";
  tests = import ../../../config/lib/btrc.nix { inherit pkgs inputs; } {
    name = "trusted-network-policy-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../RulesTests.btrc ../btrc.toml ../../../config/system/nixos/networking ];
    };
    programs.networking-tests = "tests/networking/RulesTests.btrc";
    postInstall = ''"$out/bin/networking-tests"'';
  };
in assert (import ../eval.nix { inherit system; }).roamingProfiles; tests
