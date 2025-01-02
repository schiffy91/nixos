{ lib, ... }: {
  services.xserver.enable = lib.mkDefault false;
  services.desktopManager.plasma6.enable = true;
  services.displayManager.sddm = lib.mkDefault {
    enable = true;
    wayland.enable = true;
  };
  hardware.graphics.enable = true;
}
