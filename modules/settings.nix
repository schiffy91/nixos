{ lib, config, ... }: let mkSetting = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; in {
  options = with lib.types; {
    ##### Secrets ##### 
    settings.secrets.path = mkSetting str "/etc/nixos/secrets";
    settings.secrets.hashedPasswordFile = mkSetting str "hashed_password.txt";
    ##### Admin ##### 
    settings.user.admin.username = mkSetting str "alexanderschiffhauer"; # OVERRIDE (HERE)
    settings.user.admin.autoLogin.enable = mkSetting bool false;
    settings.user.admin.autoLock.enable = mkSetting bool true;
    settings.user.admin.autoUnlockWallet.enabled = mkSetting bool true;
    settings.user.admin.authorizedKey = mkSetting str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI"; # OVERRIDE (HERE)
    ##### Disk ##### 
    settings.disk.device = mkSetting str ""; #OVERRIDE (HOST)
    settings.disk.swapSize = mkSetting str ""; #OVERRIDE (HOST)
    ##### Disk: Labels #####
    settings.disk.label.nixos = mkSetting str "nixos";
    settings.disk.label.boot = mkSetting str "boot"; # /dev/disk-by-partlabel/disk-nixos-boot
    settings.disk.label.root = mkSetting str "root"; # /dev/disk-by-partlabel/disk-nixos-root
    ##### Disk: Subvolumes & Immutability #####
    settings.disk.immutability.enable = mkSetting bool true;
    settings.disk.subvolumes.boot.mountpoint = mkSetting str "/boot";
    settings.disk.subvolumes.root.name = mkSetting str "/root";
    settings.disk.subvolumes.root.mountpoint = mkSetting str "/";
    settings.disk.subvolumes.root.preserveDirectories = mkSetting (listOf str) [
      "/etc/nixos"
      "/etc/NetworkManager/system-connections"
    ];
    settings.disk.subvolumes.home.name = mkSetting str "/home";
    settings.disk.subvolumes.home.mountpoint = mkSetting str "/home";
    settings.disk.subvolumes.home.preserveDirectories = mkSetting (listOf str) [
      "/${config.settings.user.admin.username}/nixos"
    ];
    settings.disk.subvolumes.nix.name = mkSetting str "/nix";
    settings.disk.subvolumes.nix.mountpoint = mkSetting str "/nix";
    settings.disk.subvolumes.var.name = mkSetting str "/var";
    settings.disk.subvolumes.var.mountpoint = mkSetting str "/var";
    settings.disk.subvolumes.var.preserveDirectories = mkSetting (listOf str) [
      "log"
      "lib/bluetooth"
      "lib/nixos"
      "lib/systemd/coredump"
    ];
    settings.disk.subvolumes.swap.name = mkSetting str "/swap";
    settings.disk.subvolumes.swap.mountpoint = mkSetting str "/.swapvol";
    ##### Disk: Immutability #####
    ##### Disk: Encryption #####
    settings.disk.encryption.enable = mkSetting bool true;
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/tmp/plain_text_password.txt";
    ##### Boot ##### 
    settings.boot.method = mkSetting (enum [ "Disk-Operation" "Standard-Boot" "Secure-Boot"]) "Standard-Boot"; #OVERRIDE (FLAKE)
    settings.boot.pkiBundle = mkSetting str "/var/lib/sbctl";
    ##### TPM ##### 
    settings.tpm.device = mkSetting str "/dev/tpmrm0"; #OVERRIDE (HOST)
    settings.tpm.versionPath = mkSetting str "/sys/class/tpm/tpm0/tpm_version_major";
    ##### Desktop ##### 
    settings.desktop.environment = mkSetting (enum [ "hyprland" "plasma-wayland" "plasma-x11"]) "plasma-wayland";
    settings.desktop.scalingFactor = mkSetting float 2.0;
    ##### Networking ##### 
    settings.networking.lanSubnet = mkSetting str "192.168.1.0/24"; #OVERRIDE (HOST) # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    settings.networking.ports.udp = mkSetting (listOf int) [];
    settings.networking.ports.tcp = mkSetting (listOf int) [];
    settings.networking.identityAgent = mkSetting str "~/.1password/agent.sock";
  };
}