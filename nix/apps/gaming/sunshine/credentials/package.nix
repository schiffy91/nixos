{ pkgs, user, home, usernameReference, passwordReference, publicKeys ? [] }:
let
  configuration = pkgs.writeText "sunshine-credential-references.json" (builtins.toJSON {
    inherit user home usernameReference passwordReference publicKeys;
  });
in pkgs.writeShellApplication {
  name = "nixosctl-sunshine";
  text = ''
    exec ${pkgs.python3}/bin/python3 ${./credentials.py} ${configuration} "$@"
  '';
}
