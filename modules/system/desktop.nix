{ config, pkgs, lib, ... }: lib.mkMerge [
  ##### Shared #####
  (lib.mkIf (config.settings.desktop.environment != "none") {
    hardware.graphics.enable = true;
    programs.dconf.enable =  true;
    services = {
      xserver.dpi = builtins.floor(96 * config.settings.desktop.scalingFactor);
      displayManager.enable = true;
      accounts-daemon.enable = true;
    };
  })
  ##### Wayland #####
  (lib.mkIf (lib.hasInfix "wayland" config.settings.desktop.environment) {
    environment = {
      sessionVariables = {
        NIXOS_OZONE_WL = "1"; # https://nixos.wiki/wiki/Wayland
        # NVIDIA-specific Wayland variables
        GBM_BACKEND = "nvidia-drm";
        __GLX_VENDOR_LIBRARY_NAME = "nvidia";
        # Enable NVIDIA Wayland support
        WLR_NO_HARDWARE_CURSORS = "1";  # May be needed for cursor issues
      };
      systemPackages = with pkgs; [
        wl-clipboard
      ];
    };
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
    services = {
      displayManager.sddm.enable = true;
      desktopManager.plasma6 = {
        enable = true;
        enableQt5Integration = false;
      };
    };
    environment = {
      systemPackages = with pkgs; [
        kdePackages.plasma-thunderbolt
        kdePackages.kaccounts-providers
        kdePackages.kaccounts-integration
        kdePackages.kio-gdrive
      ];
      plasma6.excludePackages = (with pkgs.kdePackages; [
        kate
        khelpcenter
        elisa
        okular
        print-manager
      ]);
    };
    security.pam.services.sddm.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;
  })
  (lib.mkIf (config.settings.desktop.environment == "plasma-x11") {
    services.displayManager = {
      sddm.wayland.enable = false;
      defaultSession = "plasmax11";
    };
  })
  (lib.mkIf (config.settings.desktop.environment == "plasma-wayland") {
    services.displayManager = {
      sddm.wayland.enable = true;
      defaultSession = "plasma";
    };
  })
  ##### Gnome #####
  (lib.mkIf (lib.hasInfix "gnome" config.settings.desktop.environment) {
    services = {
      displayManager.gdm.enable = true;
      desktopManager.gnome.enable = true;
    };
  })
  (lib.mkIf (config.settings.desktop.environment == "gnome-x11") {
    services.displayManager = {
      defaultSession = "gnome-xorg";
      gdm.wayland = false;
    };
  })
  (lib.mkIf (config.settings.desktop.environment == "gnome-wayland") {
    services.displayManager = {
      defaultSession = "gnome";
      gdm.wayland = true;
    };
  })
  ##### Hyprland #####
  (lib.mkIf (config.settings.desktop.environment == "hyprland") {
    xdg.portal = {
      enable = true;
      extraPortals = with pkgs; [ xdg.portal.extraPortals ];
    };
    environment.systemPackages = with pkgs; [ xdg-desktop-portal-hyprland ];
    services.displayManager = {
      sddm = {
        enable = true;
        wayland.enable = true;
      };
      defaultSession = "hyprland";
    };
    programs.hyprland.enable = true;
  })
]