{ lib, config, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  snapshotVolume = lib.lists.findFirst (volume: volume.flag == "snapshots") null config.settings.disk.subvolumes.volumes;
in {
  options = with lib.types; {
    settings.disk.device = mkSetting str "";
    settings.disk.boot.size = mkSetting str "4G";
    settings.disk.boot.efiSysMountPoint = mkSetting str "/boot";
    settings.disk.label.disk = mkSetting str "disk";
    settings.disk.label.main = mkSetting str "main";
    settings.disk.label.boot = mkSetting str "boot";
    settings.disk.label.recovery = mkSetting str "recovery";
    settings.disk.label.root = mkSetting str "root";
    settings.disk.partlabel.boot = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.boot}";
    settings.disk.partlabel.recovery = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.recovery}";
    settings.disk.partlabel.root = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.root}";
    settings.disk.by.partlabel.boot = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.boot}";
    settings.disk.by.partlabel.recovery = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.recovery}";
    settings.disk.by.partlabel.root = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.root}";
    settings.disk.by.mapper.root = mkSetting str "/dev/mapper/${config.settings.disk.label.root}";
    settings.disk.recovery.enable = mkSetting bool false;
    settings.disk.recovery.size = mkSetting str "4G";
    settings.disk.recovery.mountPoint = mkSetting str "/recovery";
    settings.disk.recovery.filesystemLabel = mkSetting str "RECOVERY";
    settings.disk.recovery.kernelPath = mkSetting str "EFI/recovery/kernel.efi";
    settings.disk.recovery.initrdPath = mkSetting str "EFI/recovery/initrd";
    settings.disk.recovery.entryPath = mkSetting str "loader/entries/nixos-recovery.conf";
    settings.disk.subvolumes.volumes = mkSetting (listOf (submodule{
      options = {
        name = mkSetting str null;
        mountPoint = mkSetting str null;
        mountOptions = mkSetting (listOf str) [ "compress=zstd" "noatime" ];
        neededForBoot = mkSetting bool true;
        resetOnBoot =  mkSetting bool false;
        flag = mkSetting (enum [ "none" "swap" "snapshots" "root"]) "none";
      };
    }))
    [
      { name = "@root"; mountPoint = "/"; flag = "root"; resetOnBoot = true; }
      { name = "@home"; mountPoint = "/home"; resetOnBoot = true; }
      { name = "@nix"; mountPoint = "/nix"; }
      { name = "@var"; mountPoint = "/var"; }
      { name = "@snapshots"; mountPoint = "/.snapshots"; flag = "snapshots"; }
      { name = "@swap"; mountPoint = "/.swap"; mountOptions = []; flag = "swap"; neededForBoot = false; }
    ];
    settings.disk.subvolumes.snapshots.name = mkSetting str (toString snapshotVolume.name);
    settings.disk.subvolumes.snapshots.mountPoint = mkSetting str (toString snapshotVolume.mountPoint);
    settings.disk.subvolumes.names.resetOnBoot = mkSetting str (lib.concatMapStringsSep " " (volume: volume.name) (lib.filter (volume: volume.resetOnBoot) config.settings.disk.subvolumes.volumes));
    settings.disk.subvolumes.nameMountPointPairs.resetOnBoot = mkSetting str (lib.concatMapStringsSep " " (volume: "${volume.name}=${volume.mountPoint}") (lib.filter (volume: volume.resetOnBoot) config.settings.disk.subvolumes.volumes));
    settings.disk.swap.enable = mkSetting bool true;
    settings.disk.swap.size = mkSetting str "";
    settings.disk.encryption.enable = mkSetting bool true;
    settings.disk.encryption.opportunisticUnlock.enable = mkSetting bool false;
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/run/nixosctl-install/passphrase";
    # An encrypted removable card: opened with a key file whenever it is attached, at boot or later, and mounted.
    settings.disk.sdCard.enable = mkSetting bool false;
    settings.disk.sdCard.label = mkSetting str "SD"; # the filesystem's label; the encrypted container's is "<label>-crypt"
    settings.disk.sdCard.mountPoint = mkSetting str "/sd";
    settings.disk.sdCard.keyFile = mkSetting str "/var/lib/nixos/sd-card/key"; # on the encrypted system disk, persisted, root only
  };
}
