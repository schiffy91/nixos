{ config, lib, ... }:
{
  options = {
    userConfig = lib.mkOption {
      type = lib.types.submodule {
        options = {
          rootUser = lib.mkOption {
            type = lib.types.str;
            default = "alexanderschiffhauer";
            description = "The default root user";
          };
        };
      };
      default = { rootUser = "alexanderschiffhauer"; };
      description = "Overrides related to the user configuration";
    };
  };
  config = {
    users.extraUsers.${config.userConfig.rootUser} = {
      isNormalUser = true;
      extraGroups = [ "wheel" "libvirtd" ];
      hashedPasswordFile = "/etc/nixos/secrets/hashed_password.txt"; # This line must exist, and its parent directory (i.e. secrets) must be the secrets folder
    };

    services.displayManager.autoLogin = {
      enable = true;
      user = config.userConfig.rootUser;
    };
  };
}