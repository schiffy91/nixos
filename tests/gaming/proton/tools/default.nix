{ pkgs, inputs }:
let
  programs = (import ../../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "proton-tools-contract-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../../..;
      fileset = pkgs.lib.fileset.unions [ ../../../../config/pkgs/gaming/proton/custom/tools ../../../lib ./ToolsTests.btrc ./Integration.btrc ./CommandFixture.btrc ];
    };
    programs = {
      proton-custom-tools = "config/pkgs/gaming/proton/custom/tools/Main.btrc";
      proton-tools-test = "tests/gaming/proton/tools/ToolsTests.btrc";
      proton-tools-integration = "tests/gaming/proton/tools/Integration.btrc";
      proton-command-fixture = "tests/gaming/proton/tools/CommandFixture.btrc";
    };
  };
in pkgs.runCommand "proton-tools-contract-check" {
  nativeBuildInputs = [ pkgs.rsync pkgs.coreutils pkgs.git pkgs.diffutils ];
} ''
  ${programs}/bin/proton-tools-test
  ${programs}/bin/proton-tools-integration ${programs}/bin/proton-custom-tools ${programs}/bin/proton-command-fixture
  touch "$out"
''
