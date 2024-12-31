{ lib, ... }: {
  services.xserver.enable = lib.mkDefault false;
  services.desktopManager.plasma6.enable = lib.mkDefault true;
  services.displayManager.sddm = lib.mkDefault {
    enable = lib.mkDefault true;
    wayland.enable = lib.mkDefault true;
  };
  hardware.graphics.enable = lib.mkDefault true;
}
