{ inputs, config, lib, ... }:
let 
  ##### Full Disk Encryption #####
  mkRootVolume = content: 
    if config.settings.disk.encryption.enable then {
      luks = {
        size = "100%";
        content = {
          type = "luks";
          name = config.settings.disk.label.root;
          passwordFile = config.settings.disk.encryption.plainTextPasswordFile;
          settings.allowDiscards = true;
          content = content;
        };
      };
    } else {
      "${config.settings.disk.label.root}" = {
        size = "100%";
        content = content;
      };
    };
  ##### Subvolumes #####
  mkSubvolumes = subvolumes: 
    lib.listToAttrs (map (subvolume: { 
      name = subvolume.name; 
      value = { 
        mountpoint = subvolume.mountPoint; 
        mountOptions = subvolume.mountOptions; 
      }; 
    }) (lib.filter (volume: config.settings.disk.subvolumes.swap.name != volume.name) subvolumes)) // 
    (if !config.settings.disk.swap.enable then { } else {
      "${config.settings.disk.subvolumes.swap.name}" = { 
        mountpoint = config.settings.disk.subvolumes.swap.mountPoint; 
        swap.swapfile.size = config.settings.disk.swap.size; 
      };
    });
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
        subvolumes = mkSubvolumes config.settings.disk.subvolumes.metadata;
      };
    };
  };
}