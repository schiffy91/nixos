{ pkgs }:
pkgs.writeShellApplication {
  name = "ci-local";
  runtimeInputs = [ pkgs.git pkgs.gh pkgs.python3 ];
  text = ''exec python3 -P ${./ci-local.py} "$@"'';
}
