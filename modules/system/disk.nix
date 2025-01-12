{ inputs, config, ... }:
let 
  ##### Optional Encryption Wrapper #####
  mkRootVolume = diskEncryption: content: 
    if diskEncryption then {
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
in {
  imports = [ inputs.disko.nixosModules.disko inputs.impermanence.nixosModules.impermanence ];
  ##### DISKO #####
  disko.devices = {
    disk."${config.settings.disk.label.nixos}" = {
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
          ##### Root Partition (Optional Encryption) #####
        } // mkRootVolume config.settings.disk.encryption.enable {
          type = "btrfs";
          extraArgs = [ "-f" ];
          subvolumes = {
            "${config.settings.disk.subvolumes.root.name}" = {
              mountpoint = config.settings.disk.subvolumes.root.mountpoint;
              mountOptions = [ "compress=zstd" "noatime" ];
            };
            "${config.settings.disk.subvolumes.home.name}" = {
              mountpoint = config.settings.disk.subvolumes.home.mountpoint;
              mountOptions = [ "compress=zstd" "noatime" ];
            };
            "${config.settings.disk.subvolumes.nix.name}" = {
              mountpoint = config.settings.disk.subvolumes.nix.mountpoint;
              mountOptions = [ "compress=zstd" "noatime" ];
            };
            "${config.settings.disk.subvolumes.var.name}" = {
              mountpoint = config.settings.disk.subvolumes.var.mountpoint;
              mountOptions = [ "compress=zstd" "noatime" ];
            };
          } ##### Optional Swap Volume ##### 
          // (if config.settings.disk.swapSize == "" then { } else {
            "${config.settings.disk.subvolumes.swap.name}" = { 
              mountpoint = config.settings.disk.subvolumes.swap.mountpoint; 
              swap.swapfile.size = config.settings.disk.swapSize; 
            };
          });
        };
      };
    };
  };
  ##### IMMUTABILITY ######
  fileSystems."${config.settings.disk.subvolumes.root.name}".neededForBoot = true;
  fileSystems."${config.settings.disk.subvolumes.nix.name}".neededForBoot = true;
  environment.persistence."${config.settings.disk.subvolumes.root.name}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.subvolumes.root.preserveDirectories;
    hideMounts = true;
  };
  environment.persistence."${config.settings.disk.subvolumes.home.name}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.subvolumes.home.preserveDirectories;
    hideMounts = true;
  };
  environment.persistence."${config.settings.disk.subvolumes.var.name}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.subvolumes.var.preserveDirectories;
    hideMounts = true;
  };
}