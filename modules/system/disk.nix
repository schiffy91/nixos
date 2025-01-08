{ config, inputs, ... }:
let optionalSwapVolume = if config.settings.disk.swapSize == "" then {} else {
  "/swap" = { 
    mountpoint = "/.swapvol"; 
    swap.swapfile.size = config.settings.disk.swapSize; 
  };
};
in {
  imports = [ inputs.disko.nixosModules.disko ];
  disko.devices = {
    "${config.settings.disk.label.disk}" = {
      "${config.settings.disk.label.main}" = {
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
            "${config.settings.disk.label.data}" = {
              size = "100%";
              content = {
                type = "luks";
                name = config.settings.disk.label.data;
                passwordFile = config.settings.disk.encryption.plainTextPasswordFile;
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