{ config, lib, pkgs, ... }: lib.mkIf (config.variables.desktop.displayServer == "x11") {
  services.xserver.enable = true;
  services.displayManager.defaultSession = "plasmax11";
  environment.systemPackages = with pkgs; [
    xclip
  ];
}