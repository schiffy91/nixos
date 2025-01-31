{ inputs, config, lib, ... }:
let 
  ##### Full Disk Encryption #####
  mkRootVolume = content: {
    "${config.settings.disk.label.root}" = {
      size = "100%";
      content = if !config.settings.disk.encryption.enable then content else {
        type = "luks";
        name = config.settings.disk.label.root;
        passwordFile = config.settings.disk.encryption.plainTextPasswordFile;
        settings.allowDiscards = true;
        content = content;
      };
    };
  };
  ##### Subvolumes #####
  mkSubvolumes = subvolumes: 
    lib.listToAttrs (lib.lists.forEach subvolumes (subvolume: { 
      name = subvolume.name; 
      value = { 
        mountpoint = subvolume.mountPoint;
        mountOptions = subvolume.mountOptions; 
      } // (if !config.settings.disk.swap.enable || subvolume.flag != "swap" then {} else {
        swap.swapfile.size = config.settings.disk.swap.size; 
       });
    }));
in {
  imports = [ inputs.disko.nixosModules.disko ];
  ##### Disko #####
  disko.devices.disk."${config.settings.disk.label.main}" = {
    type = "disk";
    device = config.settings.disk.device;
    content = {
      type = "gpt";
      partitions = {
        ##### Boot Partition #####
        "${config.settings.disk.label.boot}" = {
          size = "512M";
          type = "EF00";
          content = {
            type = "filesystem";
            format = "vfat";
            mountpoint = "/boot";
            mountOptions = [ "umask=0077" ];
          };
        };
      } // mkRootVolume { ##### Root Partition #####
        type = "btrfs";
        extraArgs = [ "-f" ];
        subvolumes = mkSubvolumes config.settings.disk.subvolumes.volumes;
      };
    };
  };
}