{ pkgs, inputs }:
let
  tests = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "chrome-launcher-contracts";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ./ChromeLaunchers.btrc ../lib ];
    };
    programs.chrome-launcher-tests = "tests/checks/ChromeLaunchers.btrc";
  };
  launcher = import ../../config/pkgs/utils/chrome-launchers/package.nix { inherit pkgs inputs; };
in pkgs.runCommand "chrome-launcher-check" {} ''
  ${tests}/bin/chrome-launcher-tests ${launcher}/bin/chrome-launchers
  touch "$out"
''
