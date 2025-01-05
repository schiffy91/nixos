{ config, ... }: {
  users.users.${config.variables.user.admin.username} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "${config.variables.secrets.path}/${config.variables.secrets.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "${config.variables.user.admin.authorizedKey} ${config.variables.user.admin.username}"
    ];
  };
  services.displayManager.autoLogin = {
    enable = config.variables.user.admin.autoLoginEnabled;
    user = config.variables.user.admin.username;
  };
  security.pam.sshAgentAuth.enable = true;
}