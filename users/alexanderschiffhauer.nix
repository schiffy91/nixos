{ config, pkgs, ... }: {
  home.stateVersion = "24.11";
  programs.plasma = {
    enable = true;
  };
}