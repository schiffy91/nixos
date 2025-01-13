{ inputs, config, lib, pkgs, ... }: {
  imports = [ inputs.impermanence.nixosModules.impermanence ];
  fileSystems."${config.settings.disk.subvolumes.persist.mountPoint}".neededForBoot = true;
  #boot.readOnlyNixStore = config.settings.disk.immutability.enable;
  environment.persistence."${config.settings.disk.subvolumes.persist.mountPoint}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.immutability.persist.directories;
    files = config.settings.disk.immutability.persist.files;
    hideMounts = true;
  };
  systemd.services.ephemeral-root = lib.mkIf config.settings.disk.immutability.enable {
    description = "Immutability based on BTRFS snapshots";
    wantedBy = if config.settings.disk.encryption.enable then [ "cryptsetup.target" ] else [ "local-fs.target" ];
    after = if config.settings.disk.encryption.enable then [ "cryptsetup.target" ] else [ "local-fs.target" ];
    serviceConfig = {
      Type = "oneshot";
      ExecStart = config.settings.disk.immutability.persist.scripts.postDeviceHook;
    };
  };
}