{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.semu.root = mkSetting (nullOr str) null;
    settings.apps.gaming.semu.enable = mkSetting bool (config.settings.apps.gaming.enable && pkgs.stdenv.hostPlatform.isx86_64);
    settings.apps.gaming.semu.target = mkSetting str "linux-desktop";
    settings.apps.gaming.semu.paths = mkSetting (attrsOf str) {}; # overrides the target's settings.paths
  };
}
