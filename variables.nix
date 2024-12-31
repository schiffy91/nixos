{lib, ...}:
{
  options.variables = lib.mkOption { 
    type = lib.types.attrsOf lib.types.anything; 
    default = {
      secrets = "/etc/nixos/secrets";
      user = {
        admin = "alexanderschiffhauer";
        hashedPasswordFile = "hashed_password.txt";
      };
      disk = {
        device = "OVERRIDE_THIS_VALUE_IN_HOST";
        swapSize = "OVERRIDE_THIS_VALUE_IN_HOST";
        tmpPasswordFile = "/tmp/plain_text_password.txt";
        pkiBundle = "/var/lib/sbctl";
      };
    };
  };
}
