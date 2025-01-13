{ inputs, config, lib, ... }: {
  imports = [ inputs.impermanence.nixosModules.impermanence ];
  fileSystems."${config.settings.disk.immutability.persist.mountPoint}".neededForBoot = true;
  boot.readOnlyNixStore = config.settings.disk.immutability.enable;
  environment.persistence."${config.settings.disk.immutability.persist.mountPoint}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.immutability.persist.directories;
    files = config.settings.disk.immutability.persist.files;
    hideMounts = true;
  };
  #TODO Clean up BTRFS
}