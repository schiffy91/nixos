{ config, lib, ... }:
lib.mkIf config.settings.disk.immutability.enable {
  fileSystems = lib.mkMerge (map (mountPoint: { "${mountPoint}".neededForBoot = lib.mkForce true; }) (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes));
  boot.readOnlyNixStore = true;
  boot.initrd.postResumeCommands = lib.mkAfter ''
    cat << "EOF" > /immutability.sh
    ${(builtins.readFile ../../bin/immutability.sh)}
    EOF
    chmod +x /immutability.sh
    for i in ${config.settings.disk.subvolumes.volumesNeededForBoot}; do vol=$i%=*; mount_point=/mnt/$vol;
      mount -t btrfs -o subvol=$vol ${config.settings.disk.by.partlabel.root} $mount_point; /immutability.sh $mount_point $mount_point/${config.settings.disk.immutability.persist.snapshots.name} '${lib.strings.concatStringsSep " " config.settings.disk.immutability.persist.paths}';
      umount $mount_point; done
  '';
}
