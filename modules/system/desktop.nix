{ inputs, lib, pkgs, config, ... }: lib.mkMerge [
  {
    hardware.graphics.enable = true;
    services.displayManager.sddm.enable = lib.mkDefault true;
    xdg.portal.enable = true;
  }
  (lib.mkIf (lib.hasInfix "plasma" config.settings.desktop.environment) {
    services.desktopManager.plasma6.enable = lib.mkDefault true;
    xdg.portal.extraPortals = [ pkgs.xdg-desktop-portal-kde ];
    environment.systemPackages = with pkgs; [ xdg-desktop-portal-kde ];
    environment.plasma6.excludePackages = (with pkgs.kdePackages; [
      kate
      gwenview
      khelpcenter
      elisa
      ark
      okular
      print-manager
      drkonqi
      spectacle
    ]);
  })
  (lib.mkIf (config.settings.desktop.environment == "plasma-x11") {
    services.xserver.enable = true;
    services.displayManager.defaultSession = "plasmax11";
    environment.systemPackages = with pkgs; [
      xclip
    ];
  })
  (lib.mkIf (config.settings.desktop.environment == "plasma-wayland") {
    services.displayManager.sddm.wayland.enable = true;
    services.displayManager.defaultSession = "plasma";
    environment.systemPackages = with pkgs; [
      wl-clipboard
    ];
  })
  (lib.mkIf (config.settings.desktop.environment == "hyprland") {
    services.displayManager.sddm.wayland.enable = true;
    environment.systemPackages = with pkgs; [ xdg-desktop-portal-hyprland ];
    programs.hyprland = {
      enable = true;
      package = inputs.hyprland.packages.${pkgs.stdenv.hostPlatform.system}.hyprland;
      portalPackage = inputs.hyprland.packages.${pkgs.stdenv.hostPlatform.system}.xdg-desktop-portal-hyprland;
    };
  })
]