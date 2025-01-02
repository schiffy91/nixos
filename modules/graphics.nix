{ config, lib, pkgs, ... }: {
  hardware.graphics.enable = true;
  services.xserver.enable = lib.mkDefault false;
  services.desktopManager.plasma6.enable = true;
  services.displayManager.sddm = lib.mkDefault {
    enable = !config.services.xserver.enable;
    wayland.enable = !config.services.xserver.enable;
  };
  xdg.portal = {
    enable = true;
    extraPortals = [ pkgs.xdg-desktop-portal-kde ];
  };
}
