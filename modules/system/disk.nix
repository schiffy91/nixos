{ config, inputs, ... }:
let 
mkRootVolume = diskEncryption: content: 
    if diskEncryption then {
      luks = {
        size = "100%";
        content = {
          type = "luks";
          name = config.settings.disk.label.root;
          passwordFile = config.settings.disk.encryption.plainTextPasswordFile; #TODO: This is still generated by installer.py even if disabled
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
  imports = [ inputs.disko.nixosModules.disko ];
  disko.devices.disk."${config.settings.disk.label.nixos}" = {
    type = "disk";
    device = config.settings.disk.device;
    content = {
      type = "gpt";
      partitions = {
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
      } // mkRootVolume config.settings.disk.encryption.enabled {
        type = "btrfs";
        extraArgs = [ "-f" ];
        subvolumes = {
          "/root" = {
            mountpoint = "/";
            mountOptions = [ "compress=zstd" "noatime" ];
          };
          "/home" = {
            mountpoint = "/home";
            mountOptions = [ "compress=zstd" "noatime" ];
          };
          "/nix" = {
            mountpoint = "/nix";
            mountOptions = [ "compress=zstd" "noatime" ];
          };
          "/var" = {
            mountpoint = "/var";
            mountOptions = [ "compress=zstd" "noatime" ];
          };
        } // (if config.settings.disk.swapSize == "" then { } else {
          "/swap" = { 
            mountpoint = "/.swapvol"; 
            swap.swapfile.size = config.settings.disk.swapSize; 
          };
        });
      };
    };
  };
}