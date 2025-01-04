{ config, pkgs, ... }: {
  home = {
    username = config.variables.user.admin;
    stateVersion = "24.11";
  };
}