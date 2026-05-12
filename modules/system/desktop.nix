{ config, pkgs, lib, ... }:
let
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  primaryScale = if primary == null then 1.0 else primary.scaleFactor;
in {
  hardware.graphics.enable = true;
  programs.dconf.enable = true;
  services = {
    xserver.dpi = builtins.floor (96 * primaryScale);
    displayManager = {
      enable = true;
      sddm = {
        enable = true;
        wayland.enable = true;
      };
      defaultSession = "plasma";
    };
    desktopManager.plasma6 = {
      enable = true;
      enableQt5Integration = false;
    };
    accounts-daemon.enable = true;
  };
  environment = {
    systemPackages = (with pkgs; [
      wl-clipboard
      config.settings.desktop.cursor.package
      config.settings.desktop.cursor.defaultPackage
    ]) ++ (with pkgs.kdePackages; [
      plasma-thunderbolt
      kaccounts-providers
      kaccounts-integration
      kio-gdrive
    ]);
    sessionVariables = {
      XCURSOR_THEME = config.settings.desktop.cursor.theme;
      NIXOS_OZONE_WL = "1";
    };
    plasma6.excludePackages = with pkgs.kdePackages; [
      kate
      khelpcenter
      elisa
      okular
      print-manager
    ];
  };
  security.pam.services.sddm.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;
  security.pam.services.passwd.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;  # pam_kwallet hooks chauthtok → wallet re-encrypts on passwd
}
