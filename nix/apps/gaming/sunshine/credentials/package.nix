{ pkgs, user, home, usernameReference, passwordReference }:
let
  configuration = pkgs.writeText "sunshine-credential-references.json" (builtins.toJSON {
    inherit user home usernameReference passwordReference;
  });
in pkgs.writeShellApplication {
  name = "nixosctl-sunshine";
  text = ''
    exec ${pkgs.python3}/bin/python3 ${./credentials.py} ${configuration} "$@"
  '';
}
