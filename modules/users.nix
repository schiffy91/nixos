{ config, ... }: {
  users.extraUsers.${config.variables.user.admin} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = config.variables.user.hashedPasswordFile;
  };
  services.displayManager.autoLogin = {
    enable = true;
    user = config.variables.user.admin;
  };
}