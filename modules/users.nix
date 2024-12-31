{ config, lib, ... }:
{
  users.extraUsers.${config.variables.user.admin} = lib.mkDefault {
    isNormalUser = lib.mkDefault true;
    extraGroups = lib.mkDefault [ "wheel" "libvirtd" ];
    hashedPasswordFile = lib.mkDefault config.variables.user.hashedPasswordFile;
  };
  services.displayManager.autoLogin = {
    enable = lib.mkDefault true;
    user = lib.mkDefault config.variables.user.admin;
  };
}