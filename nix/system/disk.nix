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
        inherit content;
      };
    };
  };
  ##### Subvolumes #####
  mkSubvolumes = subvolumes: 
    lib.listToAttrs (lib.lists.forEach subvolumes (subvolume: { 
      inherit (subvolume) name;
      value = { 
        mountpoint = subvolume.mountPoint;
        inherit (subvolume) mountOptions;
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
          size = config.settings.disk.boot.size;
          type = "EF00";
          content = {
            type = "filesystem";
            format = "vfat";
            mountpoint = config.settings.disk.boot.efiSysMountPoint;
            mountOptions = [ "umask=0077" ];
          };
        };
      } // lib.optionalAttrs config.settings.disk.recovery.enable {
        ##### Recovery Partition #####
        "${config.settings.disk.label.recovery}" = {
          size = config.settings.disk.recovery.size;
          type = "EA00";
          content = {
            type = "filesystem";
            format = "vfat";
            extraArgs = [ "-n" config.settings.disk.recovery.filesystemLabel ];
            mountpoint = config.settings.disk.recovery.mountPoint;
            mountOptions = [ "umask=0077" "nofail" "x-systemd.device-timeout=5s" ];
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
