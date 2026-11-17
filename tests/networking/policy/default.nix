{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes;
  pkgs = import (builtins.fetchTree { inherit (lock.nixpkgs.locked) type owner repo rev narHash; }) { inherit system; };
  inputs.btrc = builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs lock.btrc.locked [ "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev" ]));
  tests = inputs.btrc.lib.buildPackage pkgs {
    name = "trusted-network-policy-tests";
    src = (import ../../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../RulesTests.btrc ../btrc.toml ../../../config/system/nixos/networking ];
    };
    programs.networking-tests = "tests/networking/RulesTests.btrc";
    postInstall = ''"$out/bin/networking-tests"'';
  };
in assert (import ../eval.nix { inherit system; }).roamingProfiles; tests
