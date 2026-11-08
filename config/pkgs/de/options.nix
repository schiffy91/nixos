{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./kde/options.nix ];
  options = with lib.types; {
    ##### Desktop #####
    settings.desktop.enable = mkSetting bool true;
    settings.desktop.defaultSession = mkSetting str "plasma";
    settings.desktop.outputs = mkSetting (listOf (submodule {
      options = {
        name = mkSetting str null;
        scaleFactor = mkSetting float 1.0;
        primary = mkSetting bool false;
      };
    })) [];
    settings.desktop.cursor.theme = mkSetting str "Breeze";
    settings.desktop.cursor.package = mkSetting package pkgs.kdePackages.breeze;
    settings.desktop.cursor.path = mkSetting str "${config.settings.desktop.cursor.package}/share/icons/breeze_cursors";
    settings.desktop.cursor.defaultPackage = mkSetting package (pkgs.runCommandLocal "breeze-cursor-default-theme" { } ''
      mkdir -p "$out/share/icons"
      ln -s "${config.settings.desktop.cursor.path}" "$out/share/icons/default"
    '');
  };
}
