{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.assettocorsa.enable = mkSetting bool config.settings.apps.gaming.steam.protonCustom.enable;
    settings.apps.gaming.assettocorsa.mods = mkSetting (listOf package) [];
    settings.apps.gaming.assettocorsa.contentDirectory = mkSetting (nullOr str) "~/Games/Assetto Corsa";
    settings.apps.gaming.assettocorsa.presetName = mkSetting (nullOr str) null; # Link matching video, audio and control presets from contentDirectory.
    settings.apps.gaming.assettocorsa.graphicsPreset = mkSetting (nullOr (enum [ "pure-lcs-medium" ])) null;
    settings.apps.gaming.assettocorsa.wheelProfile = mkSetting (nullOr (enum [ "logitech-pro-playstation" ])) null;
    settings.apps.gaming.assettocorsa.uiScale = mkSetting (nullOr (addCheck (either int float) (value: value >= 1 && value <= 2))) null; # null follows the shared Wine DPI, within CSP's supported 100–200% range.
    settings.apps.gaming.assettocorsa.followLauncher = mkSetting bool true;
    settings.apps.gaming.assettocorsa.wheelMenu.enable = mkSetting bool false;
    settings.apps.gaming.assettocorsa.brakeGamma = mkSetting (nullOr (addCheck (either int float) (value: value >= 1 && value <= 5))) null;
    settings.apps.gaming.assettocorsa.controlBindings = mkSetting (listOf (submodule {
      options = {
        action = mkSetting str null;
        button = mkSetting (nullOr ints.unsigned) null; # null clears the wheel binding, preserving keyboard controls.
        joystick = mkSetting ints.unsigned 0;
        modifier = mkSetting (nullOr ints.unsigned) null;
      };
    })) [];
    settings.apps.gaming.assettocorsa.presets = mkSetting (listOf (submodule {
      options = {
        name = mkSetting str null;
        source = mkSetting str null;
      };
    })) [];
    settings.apps.gaming.assettocorsa.csp = mkSetting (nullOr package) null;
    settings.apps.gaming.assettocorsa.gameSettings = mkSetting (attrsOf (attrsOf (attrsOf str))) {}; # "video.ini"/"extension/*.ini" under Documents/Assetto Corsa/cfg → section → key → value.
    settings.apps.gaming.assettocorsa.steamInput = mkSetting (enum [ "disabled" "default" "enabled" ]) "disabled";
  };
}
