{ config, lib, pkgs, ... }: lib.mkIf (config.variables.desktop.displayServer == "wayland") {
  services.displayManager.sddm.wayland.enable = true;
  services.displayManager.defaultSession = "plasmawayland";
  environment.systemPackages = with pkgs; [
    wl-clipboard
  ];
}