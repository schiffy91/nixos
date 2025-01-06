{ lib, ... }: let mkVariable = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in
{
  options = with lib.types; {
    # Secrets
    variables.secrets.path = mkVariable str "/etc/nixos/secrets";
    variables.secrets.hashedPasswordFile = mkVariable str "hashed_password.txt";
    variables.secrets.initrd.rsaKeyFile = mkVariable str "ssh_host_rsa_key";
    variables.secrets.initrd.ed25519KeyFile = mkVariable str "ssh_host_ed25519_key";
    # Admin
    variables.user.admin.username = mkVariable str "alexanderschiffhauer";
    variables.user.admin.autoLoginEnabled = mkVariable bool true;
    variables.user.admin.authorizedKey = mkVariable str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI";
    # Disk
    variables.disk.device = mkVariable str ""; #OVERRIDE
    variables.disk.swapSize = mkVariable str ""; #OVERRIDE
    variables.disk.label.disk = mkVariable str "disk";
    variables.disk.label.main = mkVariable str "main";
    variables.disk.label.boot = mkVariable str "ESP";
    variables.disk.label.data = mkVariable str "luks";
    variables.disk.security.tmpPasswordPath = mkVariable str "/tmp/plain_text_password.txt";
    # Boot
    variables.boot.method = mkVariable str ""; #OVERRIDE
    variables.boot.pkiBundle = mkVariable str "/var/lib/sbctl";
    # Desktop
    variables.desktop.displayServer = mkVariable (enum [ "x11" "wayland" ]) "wayland";
    variables.desktop.scalingFactor = mkVariable float 2.0;
    # Networking
    variables.networking.lanSubnet = mkVariable str "192.168.1.0/24"; #OVERRIDE # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    variables.networking.ports.udp = mkVariable (listOf int) [];
    variables.networking.ports.tcp = mkVariable (listOf int) [];
  };
}