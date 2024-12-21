{ lib, ... }: with lib; {
  options.shared = mkOption {
    default = {};
    type = types.submodule {
      options = {
        password = mkOption {
          default = {};
          type = types.submodule {
            options = {
              permanentHashedFile = mkOption {
                type = types.str;
                default = "/etc/nixos/secrets/hashed_password.txt";
                description = "The permanent file of the hashed password for the primary user (mkpasswd -m sha-512).";
              };
              temporaryPlainTextFile = mkOption {
                type = types.str;
                default = "/tmp/plain_text_password.txt";
                description = "The plain text file used by luks to encrypt the disk. Discarded after install. Note that $ cat temporaryPlainTextFile | mkpassword -m sha-512 == cat permanentHashedFile.";
              };
            };
          };
        };
        driveConfig = mkOption {
          default = {};
          type = types.submodule {
            options = {
              swapSize = mkOption {
                type = types.str;
                default = "";
                description = "The desired size of the swap partition.";
              };
              efiSysMountPoint = mkOption {
                type = types.str;
                default = "/boot";
                description = "The mount point of the EFI System Partition.";
              };
            };
          };
        };
      };
    };
  };
}