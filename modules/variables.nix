{ lib, ... }: let mkVariable = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in
{
  options = with lib.types; {
    variables.secrets.path = mkVariable str "/etc/nixos/secrets";
    variables.secrets.hashedPasswordFile = mkVariable str "hashed_password.txt";
    variables.secrets.initrd.rsaKeyFile = mkVariable str "ssh_host_rsa_key"; # Used exclusively by boot.initrd.networking
    variables.secrets.initrd.ed25519KeyFile = mkVariable str "ssh_host_ed25519_key"; # Used exclusively by boot.initrd.networking
    variables.user.admin.username = mkVariable str "alexanderschiffhauer";
    variables.user.admin.autoLoginEnabled = mkVariable bool true;
    variables.user.admin.authorizedKey = mkVariable str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI";
    variables.disk.device = mkVariable str ""; # Per host
    variables.disk.label.disk = mkVariable str "disk";
    variables.disk.label.main = mkVariable str "main";
    variables.disk.label.boot = mkVariable str "ESP";
    variables.disk.label.encrypted = mkVariable str "luks";
    variables.disk.swapSize = mkVariable str ""; # Per host
    variables.disk.tmpPasswordPath = mkVariable str "/tmp/plain_text_password.txt";
    variables.disk.pkiBundle = mkVariable str "/var/lib/sbctl";
    variables.boot.method = mkVariable str "Standard";
    variables.desktop.displayServer = mkVariable (enum [ "x11" "wayland" ]) "wayland";
    variables.desktop.scalingFactor = mkVariable float 2.0; # Most screens are high res in 2025...
    variables.networking.lanSubnet = mkVariable str "10.0.0.0/24"; #ip -o -f inet addr show | awk '/scope global/ {print $4}'; # Override with your own subnet
    variables.networking.ports.udp = mkVariable (listOf int) [];
    variables.networking.ports.tcp = mkVariable (listOf int) [];
  };
}