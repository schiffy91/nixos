{ pkgs }:
pkgs.writeShellApplication {
  name = "nixosctl-secure-boot-keys";
  runtimeInputs = [ pkgs.python3 pkgs.openssl ];
  text = ''exec python3 ${./keys.py} "$@"'';
}
