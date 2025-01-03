{ config, ... }: {
  users.users.${config.variables.user.admin} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "${config.variables.secrets}/${config.variables.user.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "${config.variables.user.adminAuthorizedKey} ${config.variables.user.admin}"
    ];
  };
  services.displayManager.autoLogin = {
    enable = config.variables.user.adminAutoLoginEnabled;
    user = config.variables.user.admin;
  };
}