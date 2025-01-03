{ lib, ... }: let mkVariable = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in
{
  options = with lib.types; {
    variables.secrets = mkVariable str "/etc/nixos/secrets";
    variables.user.admin = mkVariable str "alexanderschiffhauer";
    variables.user.adminAutoLoginEnabled = mkVariable bool true;
    variables.user.trustedPublicKey = mkVariable str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI";
    variables.user.hashedPasswordFile = mkVariable str "hashed_password.txt";
    variables.disk.device = mkVariable str ""; # Per host
    variables.disk.swapSize = mkVariable str ""; # Per host
    variables.disk.tmpPasswordFile = mkVariable str "/tmp/plain_text_password.txt";
    variables.disk.pkiBundle = mkVariable str "/var/lib/sbctl";
    variables.desktop.displayServer = mkVariable (enum [ "x11" "wayland" ]) "wayland";
    variables.desktop.scalingFactor = mkVariable float 2.0; # Most screens are high res in 2025...
  };
}