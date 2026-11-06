{ pkgs }:
let
  launcher = import ../../nix/apps/gaming/launch/package.nix { inherit pkgs; };
in pkgs.runCommand "gaming-affinity-tests" {
  nativeBuildInputs = [ pkgs.python3 ];
} ''
  export GAME_RUN=${launcher}/bin/game-run
  python3 ${./affinity.py}
  touch "$out"
''
