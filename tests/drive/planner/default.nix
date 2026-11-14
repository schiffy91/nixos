{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  contract = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "drive-planner-contract";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ./Main.btrc ../../lib/Check.btrc ../../../config/pkgs/utils/drive/engine/Planner.btrc ];
    };
    programs.drive-planner-contract = "tests/drive/planner/Main.btrc";
  };
in pkgs.runCommand "drive-planner" { } ''
  ${contract}/bin/drive-planner-contract
  touch $out
''
