{ lib, ... }: let mksetting = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in
{
  options = with lib.types; {
    # Secrets
    settings.secrets.path = mksetting str "/etc/nixos/secrets";
    settings.secrets.hashedPasswordFile = mksetting str "hashed_password.txt";
    settings.secrets.initrd.rsaKeyFile = mksetting str "ssh_host_rsa_key";
    settings.secrets.initrd.ed25519KeyFile = mksetting str "ssh_host_ed25519_key";
    # Admin
    settings.user.admin.username = mksetting str "alexanderschiffhauer"; # OVERRIDE (HERE)
    settings.user.admin.autoLoginEnabled = mksetting bool false;
    settings.user.admin.authorizedKey = mksetting str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI"; # OVERRIDE (HERE)
    # Disk
    settings.disk.device = mksetting str ""; #OVERRIDE (HOST)
    settings.disk.swapSize = mksetting str ""; #OVERRIDE (HOST)
    settings.disk.label.disk = mksetting str "disk";
    settings.disk.label.main = mksetting str "main";
    settings.disk.label.boot = mksetting str "ESP";
    settings.disk.label.data = mksetting str "luks";
    settings.disk.encryption.plainTextPasswordFile = mksetting str "/tmp/plain_text_password.txt";
    # Boot
    settings.boot.method = mksetting str "Standard"; #OVERRIDE (FLAKE)
    settings.boot.pkiBundle = mksetting str "/var/lib/sbctl";
    # TPM
    settings.tpm.device = mksetting str "/dev/tpmrm0"; #OVERRIDE (HOST)
    settings.tpm.versionPath = mksetting str "/sys/class/tpm/tpm0/tpm_version_major";
    # Desktop
    settings.desktop.displayServer = mksetting (enum [ "x11" "wayland" ]) "wayland";
    settings.desktop.scalingFactor = mksetting float 2.0;
    # Networking
    settings.networking.lanSubnet = mksetting str "192.168.1.0/24"; #OVERRIDE (HOST) # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    settings.networking.ports.udp = mksetting (listOf int) [];
    settings.networking.ports.tcp = mksetting (listOf int) [];
  };
}