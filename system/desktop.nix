{ lib, pkgs, config, ... }: lib.mkMerge [ {
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
(lib.mkIf (config.variables.desktop.displayServer == "x11") {
  services.xserver.enable = true;
  services.displayManager.defaultSession = "plasmax11";
  environment.systemPackages = with pkgs; [
    xclip
  ];
})
(lib.mkIf (config.variables.desktop.displayServer == "wayland") {
  services.displayManager.sddm.wayland.enable = true;
  services.displayManager.defaultSession = "plasmawayland";
  environment.systemPackages = with pkgs; [
    wl-clipboard
  ];
})]