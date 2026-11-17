{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  contract = (inputs.btrc.lib.buildPackage pkgs) {
    name = "drive-planner-contract";
    src = (import ../../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ./Main.btrc ../../../config/pkgs/utils/drive/engine/Planner.btrc ];
    };
    programs.drive-planner-contract = "tests/drive/planner/Main.btrc";
  };
in pkgs.runCommand "drive-planner" { } ''
  ${contract}/bin/drive-planner-contract
  touch $out
''
