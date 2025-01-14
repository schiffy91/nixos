{ config, lib, pkgs, ... }: lib.mkIf config.settings.disk.immutability.enable {
  fileSystems = lib.mkMerge (map (mountPoint: { "${mountPoint}".neededForBoot = true; }) config.settings.disk.subvolumes.bootMountPoints);
  boot.readOnlyNixStore = true;
  systemd.services.immutability = {
    description = "Enforce immutability by deleting all non-NixOS files that aren't marked to persist across boots.";
    after = [ "systemd-cryptsetup@*.service" "local-fs.target" ];
    requires = [ "local-fs.target" ];
    before = [ "initrd-switch-root.target" "sysinit.target" ];
    wantedBy = [ "initrd.target" ];
    path = with pkgs; [ bash btrfs-progs ];
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
      ExecStart = ''
        ${pkgs.bash}/bin/bash -c
        ${pkgs.python3}/bin/python ${./snapshot-cleanup.py} /mnt ${lib.concatStringsSep " " config.settings.disk.immutability.persist.paths}
      '';
      StandardOutput = "journal+console";
      StandardError = "journal+console";
    };
  };
}