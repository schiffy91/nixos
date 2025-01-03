{ config, lib, pkgs, ... }: 
let
  sharedPackages = (with pkgs; [
    xdg-desktop-portal-kde 
  ]);
  waylandPackages = if !config.services.displayManager.sddm.wayland.enable then [] else (with pkgs; [ 
    wl-clipboard 
  ]);
  x11Packages = if !config.services.xserver.enable then [] else (with pkgs; [ 
    xclip 
  ]);
in {
  hardware.graphics.enable = true;
  services.desktopManager.plasma6.enable = lib.mkDefault true; #Plasma 6 used by default
  services.displayManager.sddm.enable = lib.mkDefault true; # Login Manager enabled by default
  services.xserver.enable = (config.variables.desktop.displayServer == "x11");
  services.displayManager.sddm.wayland.enable = (config.variables.desktop.displayServer == "wayland");
  xdg.portal = {
    enable = true;
    extraPortals = [ pkgs.xdg-desktop-portal-kde ];
  };
  environment.systemPackages = sharedPackages ++ waylandPackages ++ x11Packages;
}
