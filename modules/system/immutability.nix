{ inputs, config, lib, pkgs, ... }: {
  imports = [ inputs.impermanence.nixosModules.impermanence ];
  fileSystems."${config.settings.disk.subvolumes.persist.mountPoint}".neededForBoot = true;
  boot.readOnlyNixStore = config.settings.disk.immutability.enable;
  environment.persistence."${config.settings.disk.subvolumes.persist.mountPoint}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.immutability.persist.directories;
    files = config.settings.disk.immutability.persist.files;
    hideMounts = true;
  };
  boot.initrd.postResumeCommands = lib.mkIf config.settings.disk.immutability.enable ''
    ${pkgs.python3}/bin/python ${./../../bin/impermanence.py}
  '';
}