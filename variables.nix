{lib, ...}:
let
  mkString = string: lib.mkOption { type = lib.types.str; default = string; };
  mkObject = object: lib.mkOption { type = lib.types.attrsOf lib.types.anything; default = {}; options = object; };
in {
  options.variables = mkObject {
    secrets = mkString "/etc/nixos/secrets";
    user = mkObject {
      admin = mkString "alexanderschiffhauer";
      hashedPasswordFile = mkString "hashed_password.txt";
    };
    disk = mkObject {
      device = mkString "OVERRIDE_THIS_VALUE_IN_HOST";
      swapSize = mkString "OVERRIDE_THIS_VALUE_IN_HOST";
      tmpPasswordFile = mkString "/tmp/plain_text_password.txt";
      pkiBundle = mkString "/var/lib/sbctl";
    };
  };
}
