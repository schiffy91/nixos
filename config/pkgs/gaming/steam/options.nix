{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.steam.extraEnv = mkSetting (attrsOf str) {};
    settings.apps.gaming.steam.extraLibraries = mkSetting (listOf package) [];
    settings.apps.gaming.steam.enable = mkSetting bool (config.settings.apps.gaming.enable && pkgs.stdenv.hostPlatform.isx86_64);
    settings.apps.gaming.steam.protonCustom.enable = mkSetting bool config.settings.apps.gaming.steam.enable;
    settings.apps.gaming.steam.shortcuts = mkSetting (attrsOf (either str (submodule {
      options = {
        exe = lib.mkOption { type = str; }; # Keep stable: Steam derives the app ID from this and the name.
        icon = mkSetting str "";
        artwork = lib.mkOption {
          type = nullOr path;
          default = null;
          description = "Steam artwork directory containing p.png, landscape.png, _hero.png and _logo.png.";
        };
      };
    }))) {};
  };
}
