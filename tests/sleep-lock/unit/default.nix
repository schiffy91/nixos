{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
, pkgs ? import inputs.nixpkgs { inherit system; }
}:
let
  programs = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "sleep-lock-contracts";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./StateTests.btrc
        ./ProcessTests.btrc
        ../../../config/system/nixos/sleep/Main.btrc
        ../../../config/system/nixos/sleep/Controller.btrc
        ../../../config/system/nixos/sleep/State.btrc
        ../../../config/lib/BackgroundProcess.btrc
        ../../../config/pkgs/utils/nixosctl/lib/RootSecretFile.btrc
      ];
    };
    programs = {
      sleep-lock-tests = "tests/sleep-lock/unit/StateTests.btrc";
      background-process-tests = "tests/sleep-lock/unit/ProcessTests.btrc";
      sleep-lock = "config/system/nixos/sleep/Main.btrc";
    };
  };
in pkgs.runCommand "sleep-lock-state-tests" {
  helperContracts = import ../helpers { inherit pkgs inputs; };
} ''
  ${programs}/bin/sleep-lock-tests
  ${programs}/bin/background-process-tests
  touch "$out"
''
