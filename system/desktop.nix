{ lib, pkgs, ... }: {
  hardware.graphics.enable = true;
  services.desktopManager.plasma6.enable = lib.mkDefault true;
  services.displayManager.sddm.enable = lib.mkDefault true;
  xdg.portal = {
    enable = true;
    extraPortals = [ pkgs.xdg-desktop-portal-kde ];
  };
  environment.systemPackages = with pkgs; [
    xdg-desktop-portal-kde
  ];
}