{ pkgs, inputs }:
let
  launcher = import ../../config/pkgs/gaming/game-run/package.nix { inherit pkgs; };
  tests = (inputs.btrc.lib.buildPackage pkgs) {
    name = "gaming-affinity-probe";
    src = (import ../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../.;
      fileset = pkgs.lib.fileset.unions [ ./affinity ];
    };
    programs.affinity-tests = "gaming/affinity/Main.btrc";
  };
in pkgs.runCommand "gaming-affinity-tests" { nativeBuildInputs = [ pkgs.coreutils ]; } ''
  ${tests}/bin/affinity-tests ${launcher}/bin/game-run
  touch "$out"
''
