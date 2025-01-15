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
        
      '';
      StandardOutput = "journal+console";
      StandardError = "journal+console";
    };
  };
}