{ pkgs }:
let
  launcher = import ../../config/pkgs/gaming/game-run/package.nix { inherit pkgs; };
in pkgs.runCommand "gaming-affinity-tests" {
  nativeBuildInputs = [ pkgs.python3 ];
} ''
  export GAME_RUN=${launcher}/bin/game-run
  python3 ${./affinity.py}
  touch "$out"
''
