{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.steam.extraEnv = mkSetting (attrsOf str) {};
    settings.apps.steam.extraLibraries = mkSetting (listOf package) [];
    settings.apps.steam.enable = mkSetting bool true;
    settings.apps.steam.protonCustom.enable = mkSetting bool true;
    settings.apps.steam.shortcuts = mkSetting (attrsOf (either str (submodule {
      options = {
        exe = lib.mkOption { type = str; }; # Keep stable: Steam derives the app ID from this and the name.
        icon = mkSetting str "";
      };
    }))) {};
  };
}
