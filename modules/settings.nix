{ lib, config, ... }: let mkSetting = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in {
  options = with lib.types; {
    ##### Secrets ##### 
    settings.secrets.path = mkSetting str "/etc/nixos/secrets";
    settings.secrets.hashedPasswordFile = mkSetting str "hashed_password.txt";
    ##### Admin ##### 
    settings.user.admin.username = mkSetting str "alexanderschiffhauer";
    settings.user.admin.autoLogin.enable = mkSetting bool false;
    settings.user.admin.autoLock.enable = mkSetting bool true;
    settings.user.admin.autoUnlockWallet.enabled = mkSetting bool true;
    settings.user.admin.authorizedKey = mkSetting str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI";
    ##### Disk ##### 
    settings.disk.device = mkSetting str "";
    settings.disk.swap.enable = mkSetting bool true;
    ##### Disk: Labels #####
    settings.disk.label.nixos = mkSetting str "nixos";
    settings.disk.label.boot = mkSetting str "boot"; # /dev/disk-by-partlabel/disk-nixos-boot
    settings.disk.label.root = mkSetting str "root"; # /dev/disk-by-partlabel/disk-nixos-root
    ##### Disk: Immutability #####
    settings.disk.immutability.enable = mkSetting bool true;
    settings.disk.immutability.persist.mountPoint = "/nvm";
    settings.disk.immutability.persist.directories = mkSetting (listOf str) [
      "/etc/nixos"
      "/var/log"
      "/var/lib/bluetooth"
      "/var/lib/nixos"
      "/var/lib/systemd/coredump"
      "${config.settings.boot.pkiBundle}"
    ];
    settings.disk.immutability.persist.files = mkSetting (listOf str) [];
    ##### Disk: Subvolumes #####
    settings.disk.subvolumes = mkSetting (listOf (submodule {
      options = {
        name = mkSetting str null;
        mountPoint = mkSetting str null;
        mountOptions = mkSetting (listOf str) [ "compress=zstd" "noatime" ];
      };
    })) 
    [
      { name = "/root"; mountPoint = "/"; }
      { name = "/home"; mountPoint = "/home"; }
      { name = "/nix"; mountPoint = "/nix"; }
      { name = "/var"; mountPoint = "/var"; }
      { name = "${settings.disk.immutability.persist.mountPoint}"; mountPoint = "${settings.disk.immutability.persist.mountPoint}"; }
    ];
    ##### Disk: Swap #####
    settings.disk.swap.name = mkSetting str "/swap";
    settings.disk.swap.mountPoint = mkSetting str "/.swapvol";
    settings.disk.swap.size = mkSetting str "";
    ##### Disk: Encryption #####
    settings.disk.encryption.enable = mkSetting bool true;
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/tmp/plain_text_password.txt";
    ##### Boot ##### 
    settings.boot.method = mkSetting (enum [ "Disk-Operation" "Standard-Boot" "Secure-Boot"]) "Standard-Boot";
    settings.boot.pkiBundle = mkSetting str "/var/lib/sbctl";
    ##### TPM ##### 
    settings.tpm.device = mkSetting str "/dev/tpmrm0";
    settings.tpm.versionPath = mkSetting str "/sys/class/tpm/tpm0/tpm_version_major";
    ##### Desktop ##### 
    settings.desktop.environment = mkSetting (enum [ "hyprland" "plasma-wayland" "plasma-x11"]) "plasma-wayland";
    settings.desktop.scalingFactor = mkSetting float 2.0;
    ##### Networking ##### 
    settings.networking.lanSubnet = mkSetting str "192.168.1.0/24"; # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    settings.networking.ports.udp = mkSetting (listOf int) [];
    settings.networking.ports.tcp = mkSetting (listOf int) [];
    settings.networking.identityAgent = mkSetting str "~/.1password/agent.sock";
  };
}