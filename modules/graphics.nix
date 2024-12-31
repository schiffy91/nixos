{ lib, ... }: {
  services.xserver.enable = false;
  services.desktopManager.plasma6.enable = true;
  services.displayManager.sddm = lib.mkDefault {
    enable = true;
    wayland.enable = true;
  };
  hardware.graphics.enable = true;
}
