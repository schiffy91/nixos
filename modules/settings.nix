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
    ##### Disk: Labels #####
    settings.disk.label.main = mkSetting str "main";
    settings.disk.label.boot = mkSetting str "boot";
    settings.disk.label.root = mkSetting str "root";
    settings.disk.by.partlabel.boot = mkSetting str "/dev/disk/by-partlabel/disk-${config.settings.disk.label.main}-${config.settings.disk.label.boot}";
    settings.disk.by.partlabel.root = mkSetting str "/dev/disk/by-partlabel/disk-${config.settings.disk.label.main}-${config.settings.disk.label.root}";
    ##### Disk: Subvolumes #####
    settings.disk.subvolumes.root.name = mkSetting str "root";
    settings.disk.subvolumes.root.mountPoint = mkSetting str "/";
    settings.disk.subvolumes.home.name = mkSetting str "home";
    settings.disk.subvolumes.home.mountPoint = mkSetting str "/home";
    settings.disk.subvolumes.nix.name = mkSetting str "nix";
    settings.disk.subvolumes.nix.mountPoint = mkSetting str "/nix";
    settings.disk.subvolumes.var.name = mkSetting str "var";
    settings.disk.subvolumes.var.mountPoint = mkSetting str "/var";
    settings.disk.subvolumes.persist.name = mkSetting str "nvm";
    settings.disk.subvolumes.persist.mountPoint = mkSetting str "/nvm";
    settings.disk.subvolumes.swap.name = mkSetting str "swap";
    settings.disk.subvolumes.swap.mountPoint = mkSetting str "/swap";
    settings.disk.subvolumes.metadata = mkSetting (listOf (submodule {
      options = {
        name = mkSetting str null;
        mountPoint = mkSetting str null;
        mountOptions = mkSetting (listOf str) [ "compress=zstd" "noatime" ];
        neededForBoot = mkSetting bool false;
      };
    })) 
    [
      { name = config.settings.disk.subvolumes.root.name; mountPoint = config.settings.disk.subvolumes.root.mountPoint; neededForBoot = true; }
      { name = config.settings.disk.subvolumes.home.name; mountPoint = config.settings.disk.subvolumes.home.mountPoint; neededForBoot = true; }
      { name = config.settings.disk.subvolumes.nix.name; mountPoint = config.settings.disk.subvolumes.nix.mountPoint; }
      { name = config.settings.disk.subvolumes.var.name; mountPoint = config.settings.disk.subvolumes.var.mountPoint; neededForBoot = true; }
      { name = config.settings.disk.subvolumes.persist.name; mountPoint = config.settings.disk.subvolumes.persist.mountPoint; neededForBoot = true; }
      { name = config.settings.disk.subvolumes.swap.name; mountPoint = config.settings.disk.subvolumes.swap.mountPoint; }
    ];
    settings.disk.subvolumes.neededForBoot = mkSetting str (
      lib.concatMapStrings (volume: "${volume.name} ")
        (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.metadata)
    );
    ##### Disk: Swap #####
    settings.disk.swap.enable = mkSetting bool true;
    settings.disk.swap.size = mkSetting str "";
    ##### Disk: Encryption #####
    settings.disk.encryption.enable = mkSetting bool true;
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/tmp/plain_text_password.txt";
    ##### Disk: Immutability #####
    settings.disk.immutability.enable = mkSetting bool true;
    settings.disk.immutability.persist.directories = mkSetting (listOf str) [
      "/etc/nixos"
      "/var/log"
      "/var/lib/bluetooth"
      "/var/lib/nixos"
      "/var/lib/systemd/coredump"
      "${config.settings.boot.pkiBundle}"
    ];
    settings.disk.immutability.persist.files = mkSetting (listOf str) [
      "/etc/machine-id"
    ];
    settings.disk.immutability.persist.snapshotsPath = mkSetting str "${config.settings.disk.subvolumes.persist.mountPoint}/snapshots";
    settings.disk.immutability.persist.scripts.postCreateHook = mkSetting str ''
    (
      btrfs_mnt=$(mktemp -d)
      mount ${config.settings.disk.by.partlabel.root} "''${btrfs_mnt}" -o subvol=${config.settings.disk.subvolumes.root.mountPoint}
      trap "umount ''${btrfs_mnt}; rm -rf ''${btrfs_mnt}" EXIT

      for volume in ${config.settings.disk.subvolumes.neededForBoot}; do
        mkdir -p "''${btrfs_mnt}${config.settings.disk.immutability.persist.snapshotsPath}/''${volume}" 
        btrfs subvolume snapshot -r "''${btrfs_mnt}/''${volume}" "''${btrfs_mnt}${config.settings.disk.immutability.persist.snapshotsPath}/''${volume}/new"
      done
    )
    '';
    settings.disk.immutability.persist.scripts.postDeviceHook = mkSetting str ''
    (
      btrfs_mnt=$(mktemp -d)
      mount ${config.settings.disk.by.partlabel.root} "''${btrfs_mnt}" -o subvol=${config.settings.disk.subvolumes.root.mountPoint}
      trap "umount ''${btrfs_mnt}; rm -rf ''${btrfs_mnt}" EXIT

      delete_subvolume_recursively() {
        IFS=$'\n'
        for volume in $(btrfs subvolume list -o "''${1}" | cut -d ' ' -f 9-); do
          delete_subvolume_recursively "''${btrfs_mnt}/''${volume}"
        done
        btrfs subvolume delete "''${1}"
      }

      for volume in ${config.settings.disk.subvolumes.neededForBoot}; do
        if [[ -e "''${btrfs_mnt}/''${volume}" ]] && [[ -e "''${btrfs_mnt}${config.settings.disk.immutability.persist.snapshotsPath}/''${volume}/new" ]]; then
            timestamp=$(date --date="@$(stat -c %Y ''${btrfs_mnt}/''${volume})" "+%Y-%m-%-d_%H:%M:%S")
            mv "''${btrfs_mnt}/''${volume}" "''${btrfs_mnt}${config.settings.disk.immutability.persist.snapshotsPath}/''${volume}/''${timestamp}"
        fi

        btrfs subvolume snapshot "''${btrfs_mnt}${config.settings.disk.immutability.persist.snapshotsPath}/''${volume}/new" "''${btrfs_mnt}/''${volume}"

        for snapshot in $(find "''${btrfs_mnt}${config.settings.disk.immutability.persist.snapshotsPath}/''${volume}/" -maxdepth 1 -mtime +30 -not -name new); do
          delete_subvolume_recursively "''${snapshot}"
        done
      done
    )
    '';
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