{ config, lib, pkgs, ... }: lib.mkIf (config.settings.apps.dev.zsh.enable) {
  programs.zsh = {
    enable = true;
    enableGlobalCompInit = false; # Home Manager runs compinit in ~/.zshrc
  };
  users.users.${config.settings.users.admin.username}.shell = pkgs.zsh; # programs.zsh never sets the login shell
}
