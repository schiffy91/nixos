{ pkgs, inputs }:
let
  programs = (import ../../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "proton-tools-contract-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../../..;
      fileset = pkgs.lib.fileset.unions [ ../../../../config/pkgs/gaming/proton/custom/tools ./ToolsTests.btrc ];
    };
    programs = {
      proton-custom-tools = "config/pkgs/gaming/proton/custom/tools/Main.btrc";
      proton-tools-test = "tests/gaming/proton/tools/ToolsTests.btrc";
    };
  };
in pkgs.runCommand "proton-tools-contract-check" {
  nativeBuildInputs = [ pkgs.python3 pkgs.rsync pkgs.coreutils pkgs.git pkgs.diffutils ];
} ''
  ${programs}/bin/proton-tools-test
  python3 ${./test.py} ${programs}/bin/proton-custom-tools
  touch "$out"
''
