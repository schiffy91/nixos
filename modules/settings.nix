{ lib, config, ... }: 
let mkSetting = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; 
in {
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
    ##### Disk: Labels #####
    settings.disk.label.disk = mkSetting str "disk";
    settings.disk.label.main = mkSetting str "main";
    settings.disk.label.boot = mkSetting str "boot";
    settings.disk.label.root = mkSetting str "root";
    settings.disk.partlabel.boot = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.boot}";
    settings.disk.partlabel.root = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.root}";
    settings.disk.by.partlabel.boot = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.boot}";
    settings.disk.by.partlabel.root = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.root}";
    ##### Disk: Subvolumes #####
    settings.disk.subvolumes.volumes = mkSetting (listOf (submodule{ 
      options = { 
        name = mkSetting str null; 
        mountPoint = mkSetting str null; 
        mountOptions = mkSetting (listOf str) [ "compress=zstd" "noatime" ]; 
        neededForBoot = mkSetting bool false;
        flag = mkSetting (enum [ "none" "swap" "snapshots" "root"]) "none";
      };
    }))
    [
      { name = "@root"; mountPoint = "/"; neededForBoot = true; flag = "root"; }
      { name = "@home"; mountPoint = "/home"; neededForBoot = true; }
      { name = "@nix"; mountPoint = "/nix"; }
      { name = "@var"; mountPoint = "/var"; neededForBoot = true; }
      { name = "@snapshots"; mountPoint = "/.snapshots"; neededForBoot = true; flag = "snapshots"; }
      { name = "@swap"; mountPoint = "/.swap"; mountOptions = []; flag = "swap"; }
    ];
    settings.disk.subvolumes.root.name = mkSetting str (toString ((lib.lists.findFirst (volume: volume.flag == "root") null config.settings.disk.subvolumes.volumes).name));
    settings.disk.subvolumes.snapshots.name = mkSetting str (toString ((lib.lists.findFirst (volume: volume.flag == "snapshots") null config.settings.disk.subvolumes.volumes).name));
    settings.disk.subvolumes.snapshots.mountPoint = mkSetting str (toString ((lib.lists.findFirst (volume: volume.flag == "snapshots") null config.settings.disk.subvolumes.volumes).mountPoint));
    settings.disk.subvolumes.volumesNeededForBoot = mkSetting str (
      lib.concatMapStrings (volume: "${volume.name}=${volume.mountPoint} ") (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes)
    );
    ##### Disk: Swap #####
    settings.disk.swap.enable = mkSetting bool true;
    settings.disk.swap.size = mkSetting str "";
    ##### Disk: Encryption #####
    settings.disk.encryption.enable = mkSetting bool true;
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/tmp/plain_text_password.txt";
    ##### Disk: Immutability #####
    settings.disk.immutability.enable = mkSetting bool false;
    settings.disk.immutability.persist.snapshots.cleanRoot = mkSetting str "CLEAN_ROOT";
    settings.disk.immutability.persist.paths = mkSetting (listOf str) [
      "/etc/nixos"
      "/etc/machine-id"
      "/var/log"
      "/var/lib/bluetooth"
      "/var/lib/nixos"
      "/var/lib/systemd/coredump"
      "${config.settings.boot.pkiBundle}"
    ];
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