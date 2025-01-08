{ config, ... }: {
  users.users.${config.settings.user.admin.username} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "${config.settings.secrets.path}/${config.settings.secrets.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "${config.settings.user.admin.authorizedKey} ${config.settings.user.admin.username}"
    ];
  };
  services.displayManager.autoLogin = {
    enable = config.settings.user.admin.autoLoginEnabled;
    user = config.settings.user.admin.username;
  };
}