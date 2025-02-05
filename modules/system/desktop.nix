{ config, pkgs, pkgs-unstable, lib, ... }: lib.mkMerge [{
  ##### Shared Settings #####
    hardware.graphics.enable = true;
    services.displayManager.sddm.enable = lib.mkDefault true;
    xdg.portal.enable = true;
    programs.dconf.enable =  true;
    services.xserver.dpi = builtins.floor(96 * config.settings.desktop.scalingFactor);
  }
  ##### Shared Plasma Settings #####
  (lib.mkIf (lib.hasInfix "plasma" config.settings.desktop.environment) {
    services.desktopManager.plasma6.enable = lib.mkDefault true;
    xdg.portal.extraPortals = [ pkgs.xdg-desktop-portal-kde ];
    environment.systemPackages = with pkgs; [
      xdg-desktop-portal-kde
    ];
    environment.plasma6.excludePackages = (with pkgs.kdePackages; [
      kate
      khelpcenter
      elisa
      okular
      print-manager
    ]);
    security.pam.services.sddm.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;
    ##### Scaling #####
    environment.sessionVariables = lib.mkIf (lib.hasInfix "plasma" config.settings.desktop.environment) {
        #"QT_AUTO_SCREEN_SCALE_FACTOR" = "1";
        #"QT_SCALE_FACTOR" = toString config.settings.desktop.scalingFactor;
        #"PLASMA_USE_QT_SCALING" = "1";
      };
  })
  ##### Plasma X11 Settings #####
  (lib.mkIf (config.settings.desktop.environment == "plasma-x11") {
    services.xserver.enable = true;
    services.displayManager.defaultSession = "plasmax11";
    environment.systemPackages = with pkgs; [
      xclip
    ];
  })
  ##### Plasma Wayland Settings #####
  (lib.mkIf (config.settings.desktop.environment == "plasma-wayland") {
    environment.sessionVariables.NIXOS_OZONE_WL = "1"; # https://nixos.wiki/wiki/Wayland
    services.displayManager.defaultSession = "plasma";
    services.displayManager.sddm.wayland.enable = true;
    environment.systemPackages = with pkgs; [
      wl-clipboard
    ];
  })
  ##### Hyprland Settings #####
  (lib.mkIf (config.settings.desktop.environment == "hyprland") {
    environment.systemPackages = with pkgs; [ xdg-desktop-portal-hyprland ];
    services.displayManager.defaultSession = "hyprland";
    services.displayManager.sddm.wayland.enable = true;
    programs.hyprland = {
      enable = true;
      package = pkgs-unstable.hyprland;
    };
  })
]