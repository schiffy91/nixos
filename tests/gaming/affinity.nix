{ pkgs, inputs }:
let
  launcher = import ../../config/pkgs/gaming/game-run/package.nix { inherit pkgs; };
  tests = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "gaming-affinity-probe";
    src = pkgs.lib.fileset.toSource {
      root = ../.;
      fileset = pkgs.lib.fileset.unions [ ./affinity ../lib ];
    };
    programs.affinity-tests = "gaming/affinity/Main.btrc";
  };
in pkgs.runCommand "gaming-affinity-tests" { nativeBuildInputs = [ pkgs.coreutils ]; } ''
  ${tests}/bin/affinity-tests ${launcher}/bin/game-run
  touch "$out"
''
