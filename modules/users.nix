{ config, ... }: {
  users.users.${config.variables.user.admin} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "${config.variables.secrets}/${config.variables.user.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "${config.variables.user.public_key} ${config.variables.user.admin}"
    ];
  };
  services.displayManager.autoLogin = {
    enable = true;
    user = config.variables.user.admin;
  };
}