{ lib, ... }: let mkSetting = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in {
  options = with lib.types; {
    # Secrets
    settings.secrets.path = mkSetting str "/etc/nixos/secrets";
    settings.secrets.hashedPasswordFile = mkSetting str "hashed_password.txt";
    settings.secrets.initrd.rsaKeyFile = mkSetting str "ssh_host_rsa_key";
    settings.secrets.initrd.ed25519KeyFile = mkSetting str "ssh_host_ed25519_key";
    # Admin
    settings.user.admin.username = mkSetting str "alexanderschiffhauer"; # OVERRIDE (HERE)
    settings.user.admin.autoLoginEnabled = mkSetting bool false;
    settings.user.admin.authorizedKey = mkSetting str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI"; # OVERRIDE (HERE)
    # Disk
    settings.disk.device = mkSetting str ""; #OVERRIDE (HOST)
    settings.disk.swapSize = mkSetting str ""; #OVERRIDE (HOST)
    settings.disk.label.nixos = mkSetting str "nixos";
    settings.disk.label.boot = mkSetting str "boot"; # /dev/disk-by-partlabel/disk-nixos-boot
    settings.disk.label.root = mkSetting str "root"; # /dev/disk-by-partlabel/disk-nixos-root
    settings.disk.encryption.enabled = mkSetting bool true; #TODO Implement this
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/tmp/plain_text_password.txt";
    # Boot
    settings.boot.method = mkSetting str "Standard"; #OVERRIDE (FLAKE)
    settings.boot.pkiBundle = mkSetting str "/var/lib/sbctl";
    # TPM
    settings.tpm.device = mkSetting str "/dev/tpmrm0"; #OVERRIDE (HOST)
    settings.tpm.versionPath = mkSetting str "/sys/class/tpm/tpm0/tpm_version_major";
    # Desktop
    settings.desktop.environment = mkSetting (enum [ "hyprland" "plasma-wayland" "plasma-x11"]) "plasma-wayland";
    settings.desktop.scalingFactor = mkSetting float 2.0;
    # Networking
    settings.networking.lanSubnet = mkSetting str "192.168.1.0/24"; #OVERRIDE (HOST) # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    settings.networking.ports.udp = mkSetting (listOf int) [];
    settings.networking.ports.tcp = mkSetting (listOf int) [];
  };
}