{ config, pkgs, ... }: {
  home.stateVersion = "24.11";
  home.packages = [ pkgs.firefox ];
}