{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.assettocorsa.enable = mkSetting bool true;
    settings.apps.assettocorsa.mods = mkSetting (listOf package) [];
    settings.apps.assettocorsa.contentDirectory = mkSetting (nullOr str) "~/Games/Assetto Corsa";
    settings.apps.assettocorsa.uiScale = mkSetting (nullOr (addCheck (either int float) (value: value >= 1 && value <= 2))) null; # null follows the shared Wine DPI, within CSP's supported 100–200% range.
    settings.apps.assettocorsa.followLauncher = mkSetting bool true;
    settings.apps.assettocorsa.wheelMenu.enable = mkSetting bool false;
    settings.apps.assettocorsa.brakeGamma = mkSetting (nullOr (addCheck (either int float) (value: value >= 1 && value <= 5))) null;
    settings.apps.assettocorsa.controlBindings = mkSetting (listOf (submodule {
      options = {
        action = mkSetting str null;
        button = mkSetting (nullOr ints.unsigned) null; # null clears the wheel binding, preserving keyboard controls.
        joystick = mkSetting ints.unsigned 0;
        modifier = mkSetting (nullOr ints.unsigned) null;
      };
    })) [];
    settings.apps.assettocorsa.presets = mkSetting (listOf (submodule {
      options = {
        name = mkSetting str null;
        source = mkSetting str null;
      };
    })) [];
    settings.apps.assettocorsa.csp = mkSetting (nullOr package) null;
    settings.apps.assettocorsa.gameSettings = mkSetting (attrsOf (attrsOf (attrsOf str))) {}; # "video.ini"/"extension/*.ini" under Documents/Assetto Corsa/cfg → section → key → value.
    settings.apps.assettocorsa.steamInput = mkSetting (enum [ "disabled" "default" "enabled" ]) "disabled";
  };
}
