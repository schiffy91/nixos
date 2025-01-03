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
  services.xserver.enable = lib.mkDefault false; #X11 disabled by default
  services.displayManager.sddm.wayland.enable = !config.services.xserver.enable; # Wayland is used if X11 is disabled
  xdg.portal = {
    enable = true;
    extraPortals = [ pkgs.xdg-desktop-portal-kde ];
  };
  environment.systemPackages = sharedPackages ++ waylandPackages ++ x11Packages;
}
