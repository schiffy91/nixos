{ lib, ... }: let mkVariable = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in
{
  options = with lib.types; {
    variables.secrets = mkVariable str "/etc/nixos/secrets";
    variables.user.admin = mkVariable str "alexanderschiffhauer";
    variables.user.hashedPasswordFile = mkVariable str "hashed_password.txt";
    variables.disk.device = mkVariable str "OVERRIDE_THIS_VALUE_IN_HOST";
    variables.disk.swapSize = mkVariable str "OVERRIDE_THIS_VALUE_IN_HOST";
    variables.disk.tmpPasswordFile = mkVariable str "/tmp/plain_text_password.txt";
    variables.disk.pkiBundle = mkVariable str "/var/lib/sbctl";
  };
}