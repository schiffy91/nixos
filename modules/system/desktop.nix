{ config, pkgs, pkgs-unstable, lib, ... }: lib.mkMerge [{
  ##### Shared #####
    hardware.graphics.enable = true;
    programs.dconf.enable =  true;
    services.xserver.dpi = builtins.floor(96 * config.settings.desktop.scalingFactor);
  }
  ##### Wayland #####
  (lib.mkIf (lib.hasInfix "wayland" config.settings.desktop.environment) {
    environment.sessionVariables.NIXOS_OZONE_WL = "1"; # https://nixos.wiki/wiki/Wayland
    environment.systemPackages = with pkgs; [
      wl-clipboard
    ];
  })
  ##### X11 #####
  (lib.mkIf (lib.hasInfix "x11" config.settings.desktop.environment) {
    services.xserver.enable = true;
    environment.systemPackages = with pkgs; [
      xclip
    ];
  })
  ##### Plasma #####
  (lib.mkIf (lib.hasInfix "plasma" config.settings.desktop.environment) {
    services.displayManager.sddm.wayland.enable = true;
    services.displayManager.sddm.enable = lib.mkDefault true;
    services.desktopManager.plasma6 = {
      enable = lib.mkDefault true;
      enableQt5Integration = false;
    };
    services.accounts-daemon.enable = true;
    environment.systemPackages = with pkgs; [
      kdePackages.plasma-thunderbolt
      kdePackages.kaccounts-providers
      kdePackages.kaccounts-integration
      kdePackages.kio-gdrive
    ];
    environment.plasma6.excludePackages = (with pkgs.kdePackages; [
      kate
      khelpcenter
      elisa
      okular
      print-manager
    ]);
    security.pam.services.sddm.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;
  })
  ##### Plasma X11 #####
  (lib.mkIf (config.settings.desktop.environment == "plasma-x11") {
    services.displayManager.defaultSession = "plasmax11";
  })
  ##### Plasma Wayland #####
  (lib.mkIf (config.settings.desktop.environment == "plasma-wayland") {
    services.displayManager.defaultSession = "plasma";
  })
  ##### Gnome #####
  (lib.mkIf (lib.hasInfix "gnome" config.settings.desktop.environment) {
    services.displayManager.defaultSession = "gnome";
    services.xserver.displayManager.gdm.enable = true;
  })
  ##### Gnome Wayland #####
  (lib.mkIf (config.settings.desktop.environment == "gnome-x11") {
    services.xserver.displayManager.gdm.wayland = false;
  })
    ##### Gnome X11 #####
  (lib.mkIf (config.settings.desktop.environment == "gnome-wayland") {
    services.xserver.displayManager.gdm.wayland = true;
  })
  ##### Hyprland Settings #####
  (lib.mkIf (config.settings.desktop.environment == "hyprland") {
    xdg.portal.enable = true;
    xdg.portal.extraPortals = with pkgs; [ xdg.portal.extraPortals ];
    environment.systemPackages = with pkgs; [ xdg-desktop-portal-hyprland ];
    services.displayManager.sddm.enable = lib.mkDefault true;
    services.displayManager.defaultSession = "hyprland";
    services.displayManager.sddm.wayland.enable = true;
    programs.hyprland = {
      enable = true;
      package = pkgs-unstable.hyprland; # Always use unstable for hyprland given its bugs & pace of development
    };
  })
]