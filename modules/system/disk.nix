{ config, inputs, ... }:
let optionalSwapVolume = if config.variables.disk.swapSize == "" then {} else {
  "/swap" = { 
    mountpoint = "/.swapvol"; 
    swap.swapfile.size = config.variables.disk.swapSize; 
  };
};
in
{
  imports = [ inputs.disko.nixosModules.disko ];
  disko.devices = {
    "${config.variables.disk.label.disk}" = {
      "${config.variables.disk.label.main}" = {
        type = "disk";
        device = config.variables.disk.device;
        content = {
          type = "gpt";
          partitions = {
            "${config.variables.disk.label.boot}" = {
              size = "512M";
              type = "EF00";
              content = {
                type = "filesystem";
                format = "vfat";
                mountpoint = "/boot";
                mountOptions = [ "umask=0077" ];
              };
            };
            "${config.variables.disk.label.data}" = {
              size = "100%";
              content = {
                type = "luks";
                name = config.variables.disk.label.data;
                passwordFile = config.variables.disk.encryption.plainTextPasswordFile;
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
                  } // optionalSwapVolume;
                };
              };
            };
          };
        };
      };
    };
  };
}