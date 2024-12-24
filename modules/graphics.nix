{ ... }: {
  services.xserver.enable = false;
  services.desktopManager.plasma6.enable = true;
  services.displayManager.sddm = {
    enable = true;
    wayland.enable = true;
  };
  hardware.graphics.enable = true;
}
