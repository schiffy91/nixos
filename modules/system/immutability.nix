{ inputs, config, lib, ... }: {
  imports = [ inputs.impermanence.nixosModules.impermanence ];
  fileSystems = lib.listToAttrs (map (subvolume: { 
    name = "${subvolume.mountPoint}"; 
    value.neededForBoot = true; 
  }) config.settings.disk.subvolumes);
  environment.persistence = lib.listToAttrs (map (subvolume: {
    name = "${subvolume.mountPoint}";
    value = {
      enable = config.settings.disk.immutability.enable;
      directories = subvolume.persistDirectories;
      files = subvolume.persistFiles;
      hideMounts = true;
    };
  }) (builtins.filter (subvolume: subvolume.persistence) config.settings.disk.subvolumes));
}