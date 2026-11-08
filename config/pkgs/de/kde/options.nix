{ lib, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.users.admin.autoUnlockWallet.enabled = mkSetting bool true;
    ##### Desktop: Plasma #####
    settings.desktop.plasma.taskbarLaunchers = mkSetting (listOf str) [
      "applications:org.kde.dolphin.desktop"
      "applications:org.kde.konsole.desktop"
    ];
    settings.desktop.plasma.virtualKeyboard.enable = mkSetting bool false;
    settings.desktop.plasma.colorScheme = mkSetting str "BreezeDark";
    settings.desktop.plasma.iconTheme = mkSetting str "Papirus-Dark";
    settings.desktop.plasma.iconThemePackage = mkSetting package pkgs.papirus-icon-theme;
    settings.desktop.plasma.accentColor = mkSetting str "40,40,40";
    settings.desktop.plasma.wallpaper = mkSetting str "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
  };
}
