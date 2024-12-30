{ lib, config, inputs, ... }:
{
  imports = [ inputs.disko.nixosModules.disko ];
  options = {
    diskOverrides = lib.mkOption {
      type = lib.types.submodule {
        options = {
          device = lib.mkOption {
            type = lib.types.str;
            default = "TO_BE_CHANGED";
            description = "The device path for the main disk (e.g., /dev/sda).";
          };
          swapSize = lib.mkOption {
            type = lib.types.str;
            default = "TO_BE_CHANGED";
            description = "The size of the swap file (e.g., 16G).";
          };
        };
      };
      default = {};
      description = "Overrides for disk and swap configuration.";
    };
  };
  config = {
    disko.devices = {
      disk = {
        main = {
          type = "disk";
          device = lib.mkDefault config.diskOverrides.device;
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
                  passwordFile = "/tmp/plain_text_password.txt"; # If you don't use install.sh, create this file andrun 'mkpasswd -m sha-512 $(cat /tmp/plain_text_password.txt) > /etc/nixos/secrets/hashed_password.txt'
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
                      "/swap" = {
                        mountpoint = "/.swapvol";
                        swap.swapfile.size = config.diskOverrides.swapSize;
                      };
                      "/var" = {
                        mountpoint = "/var";
                        mountOptions = [ "compress=zstd" "noatime" ];
                      };
                    };
                  };
                };
              };
            };
          };
        };
      };
    };
  };
}