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
  services.displayManager.defaultSession = (if config.variables.desktop.displayServer == "wayland" then "plasmawayland" else "plasmax11");
  services.displayManager.sddm.enable = lib.mkDefault true; # Login Manager enabled by default
  services.xserver.enable = (config.variables.desktop.displayServer == "x11");
  services.displayManager.sddm.wayland.enable = (config.variables.desktop.displayServer == "wayland");
  services.xserver = {
    dpi = builtins.floor (96.0 * config.variables.desktop.scalingFactor);
    upscaleDefaultCursor = true;
  };
  # TODO: Test these
  environment.sessionVariables = {
    GDK_SCALE = toString config.variables.desktop.scalingFactor;
    GDK_DPI_SCALE = toString config.variables.desktop.scalingFactor;
    QT_SCALE_FACTOR = toString config.variables.desktop.scalingFactor;
    XCURSOR_SIZE = toString (24 * config.variables.desktop.scalingFactor);
  };
  xdg.portal = {
    enable = true;
    extraPortals = [ pkgs.xdg-desktop-portal-kde ];
  };
  environment.systemPackages = sharedPackages ++ waylandPackages ++ x11Packages;
}
