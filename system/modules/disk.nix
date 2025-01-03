{ config, inputs, ... }:
let 
  mkSwapVolume = size: 
  if size != "" then { 
    "/swap" = { 
      mountpoint = "/.swapvol"; 
      swap.swapfile.size = size; 
    }; 
  } else {};
in
{
  imports = [ inputs.disko.nixosModules.disko ];
  disko.devices = {
    disk = {
      main = {
        type = "disk";
        device = config.variables.disk.device;
        content = {
          type = "gpt";
          partitions = {
            ESP = {
              size = "512M";
              type = "EF00";
              content = {
                type = "filesystem";
                format = "vfat";
                mountpoint = "/boot";
                mountOptions = [ "umask=0077" ];
              };
            };
            luks = {
              size = "100%";
              content = {
                type = "luks";
                name = "crypted";
                passwordFile = config.variables.disk.tmpPasswordFile;
                settings.allowDiscards = true;
                content = {
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
                  } // (mkSwapVolume config.variables.disk.swapSize);
                };
              };
            };
          };
        };
      };
    };
  };
}