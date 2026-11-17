{ pkgs, inputs }:
let
  tests = (inputs.btrc.lib.buildPackage pkgs) {
    name = "chrome-launcher-contracts";
    src = (import ../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ./ChromeLaunchers.btrc ];
    };
    programs.chrome-launcher-tests = "tests/checks/ChromeLaunchers.btrc";
  };
  launcher = import ../../config/pkgs/utils/chrome-launchers/package.nix { inherit pkgs inputs; };
in pkgs.runCommand "chrome-launcher-check" {} ''
  ${tests}/bin/chrome-launcher-tests ${launcher}/bin/chrome-launchers
  touch "$out"
''
