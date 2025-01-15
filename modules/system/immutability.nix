{ config, pkgs, lib, ... }:

lib.mkIf config.settings.disk.immutability.enable {
  ### This merges existing fileSystems or do whatever is needed if you want to
  ### forcibly mark them as neededForBoot, read-only store, etc.
  fileSystems = lib.mkMerge (map (mountPoint: {
    "${mountPoint}".neededForBoot = true;
  }) config.settings.disk.subvolumes.bootMountPoints);

  boot.readOnlyNixStore = true;
  boot.initrd.postResumeCommands = lib.mkAfter ''
    for i in ${config.settings.disk.subvolumes.volumesNeededForBoot}; do vol=$i%=*; mount_point=/mnt/$vol;
      mount -t btrfs -o subvol=$vol ${config.settings.disk.by.partlabel.root} $mount_point; /opt/restore-subvolume.sh $mount_point $mount_point/${config.settings.disk.immutability.persist.snapshots.name} '${lib.strings.concatStringsSep " " config.settings.disk.immutability.persist.paths}';
      umount $mount_point; done
  '';
}
