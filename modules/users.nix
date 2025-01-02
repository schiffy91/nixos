{ config, ... }: {
  users.users.${config.variables.user.admin} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "${config.variables.secrets}/${config.variables.user.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI= ${config.variables.user.admin}"
    ];
  };
  services.displayManager.autoLogin = {
    enable = true;
    user = config.variables.user.admin;
  };
}