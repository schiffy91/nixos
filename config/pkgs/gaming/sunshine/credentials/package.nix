{ pkgs, inputs, user, home, usernameReference, passwordReference, publicKeys ? [] }:
let
  configuration = pkgs.writeText "sunshine-credential-references.json" (builtins.toJSON {
    inherit user home usernameReference passwordReference publicKeys;
  });
in (import ../../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "sunshine-credentials";
  src = ./src;
  programs.nixosctl-sunshine = "Main.btrc";
  wrapperArgs = [ "--add-flags" (toString configuration) ];
}
