{lib, ...}:
{
  options = {
    variables = lib.mkOption {
      description = "Variables to be overriden";
      type = lib.types.submodule {
        secrets = lib.mkOption {
          description = "Variables related to secrets storage";
          type = lib.types.str;
          default = "/etc/nixos/secrets";
        };
        user = lib.mkOption {
          description = "Variables related to the user configuration";
          type = lib.types.submodule {
            options = {
              admin = lib.mkOption {
                description = "The default admin (i.e. root) user";
                type = lib.types.str;
                default = "alexanderschiffhauer";
              };
              hashedPasswordFile = lib.mkOption {
                description = "The relative location of the admin's hashed password file with respect to the secrets.";
                type = lib.types.str;
                default = "hashed_password.txt";
              };
            };
          };
        };
        disk = lib.mkOption {
          description = "Variables related to disk device path and swap size.";
          type = lib.types.submodule {
            options = {
              device = lib.mkOption {
                description = "The device path for the main disk (e.g., /dev/sda).";
                type = lib.types.str;
                default = "YOU_HAVE_TO_OVERRIDE_THIS_VALUE_IN_YOUR_HOST.NIX_FILE";
              };
              swapSize = lib.mkOption {
                description = "The size of the swap file (e.g., 16G).";
                type = lib.types.str;
                default = "YOU_HAVE_TO_OVERRIDE_THIS_VALUE_IN_YOUR_HOST.NIX_FILE";
              };
              tmpPasswordFile = lib.mkOption {
                description = "The location of the temporary plain text password file during installation";
                type = lib.types.str;
                default = "/tmp/plain_text_password.txt";
              };
              pkiBundle = lib.mkOption {
                description = "The location of the PKI bundle";
                type = lib.types.str;
                default = "/var/lib/sbctl";
              };
            };
          };
        };
      };
    };
  };
}
