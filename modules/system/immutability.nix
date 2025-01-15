{ config, lib, pkgs, ... }: lib.mkIf config.settings.disk.immutability.enable {
  fileSystems = lib.mkMerge (map (mountPoint: { "${mountPoint}".neededForBoot = true; }) config.settings.disk.subvolumes.bootMountPoints);
  boot.readOnlyNixStore = true;
  boot.initrd.systemd.services.immutability = {
    description = "Enforce immutability at boot-time by deleting all new paths that aren't managed by NixOS or marked to persist.";
    wantedBy = [ "initrd.target" ];
    before = [ "sysroot.mount" ];
    after = if config.settings.disk.encryption.enable then [ "systemd-cryptsetup@*.service" ] else [ ];
    path = (with pkgs; [ btrfs-progs rsync ]);
    unitConfig.DefaultDependencies = "no";
    serviceConfig.Type = "oneshot";
    script = ''
      cat << "EOF" > /immutability.sh
      ${(builtins.readFile ../../bin/immutability.sh)}
      EOF
      chmod +x /immutability.sh
      for i in ${config.settings.disk.subvolumes.volumesNeededForBoot}; do vol=$i%=*; mount_point=/mnt/$vol;
        mount -t btrfs -o subvol=$vol ${config.settings.disk.by.partlabel.root} $mount_point; /immutability.sh $mount_point $mount_point/${config.settings.disk.immutability.persist.snapshots.name} '${lib.strings.concatStringsSep " " config.settings.disk.immutability.persist.paths}';
        umount $mount_point; done
    '';
  };
}