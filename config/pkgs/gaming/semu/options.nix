{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.semu.root = mkSetting (nullOr str) null;
    settings.apps.semu.enable = mkSetting bool true;
    settings.apps.semu.target = mkSetting str "linux-desktop";
    settings.apps.semu.paths = mkSetting (attrsOf str) {}; # overrides the target's settings.paths
  };
}
