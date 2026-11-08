{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./assettocorsa/options.nix ./steam/options.nix ./sunshine/options.nix ./semu/options.nix ./rocksmith/options.nix ];
  options = with lib.types; {
    settings.apps.gaming.enable = mkSetting bool true;
    settings.apps.gaming.packages = mkSetting (listOf package) [];
    settings.apps.moonlight.enable = mkSetting bool false;
    settings.apps.chiaki.enable = mkSetting bool false;
    settings.apps.gaming.cpuAffinity = mkSetting (nullOr (strMatching "[0-9]+(-[0-9]+)?(,[0-9]+(-[0-9]+)?)*")) null; # Linux CPU list; null leaves scheduling unrestricted.
    settings.apps.battlenet.enable = mkSetting bool true;
    settings.apps.wow.enable = mkSetting bool true;
    settings.apps.btrsmith.enable = mkSetting bool false;
    settings.apps.simracing.enable = mkSetting bool false;
  };
}
