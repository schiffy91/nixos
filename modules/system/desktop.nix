{ inputs, config, pkgs, lib, ... }: lib.mkMerge [
  ##### Shared Settings #####
  {
    hardware.graphics.enable = true;
    services.displayManager.sddm.enable = lib.mkDefault true;
    xdg.portal.enable = true;
  }
  ##### Shared Plasma Settings #####
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
  ##### Plasma X11 Settings #####
  (lib.mkIf (config.settings.desktop.environment == "plasma-x11") {
    services.displayManager.sddm.enable = false;
    services.xserver.enable = true;
    services.displayManager.defaultSession = "plasmax11";
    environment.systemPackages = with pkgs; [
      xclip
      xsettingsd
      xorg.xrdb
    ];
  })
  ##### Plasma Wayland Settings #####
  (lib.mkIf (config.settings.desktop.environment == "plasma-wayland") {
    environment.sessionVariables.NIXOS_OZONE_WL = "1"; # https://nixos.wiki/wiki/Wayland
    services.displayManager.defaultSession = "plasma";
    environment.systemPackages = with pkgs; [
      wl-clipboard
    ];
  })
  ##### Hyprland Settings #####
  (lib.mkIf (config.settings.desktop.environment == "hyprland") {
    environment.systemPackages = with pkgs; [ xdg-desktop-portal-hyprland ];
    programs.hyprland = {
      enable = true;
      package = inputs.hyprland.packages.${pkgs.stdenv.hostPlatform.system}.hyprland;
      portalPackage = inputs.hyprland.packages.${pkgs.stdenv.hostPlatform.system}.xdg-desktop-portal-hyprland;
    };
  })
]